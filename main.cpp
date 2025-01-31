#include <algorithm>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <random>
#include <sstream>
#include <map>
#include <omp.h>
using namespace std;

struct CircleData {
    sf::CircleShape circle;
    int depth;
};

struct PixelData {
    sf::Color pixelColour;

    PixelData() {
        pixelColour.r = 0;
        pixelColour.g = 0;
        pixelColour.b = 0;
        pixelColour.a = 0;
    }
};

struct BitMap {
    int width;
    int height;
    std::vector<std::vector<PixelData>> pixels;

    BitMap(int w, int h) : width(w), height(h) {
        pixels.resize(height, std::vector<PixelData>(width));
    }

    BitMap() : width(0), height(0) {
    }

    PixelData& getPixel(int x, int y) {
        return pixels[x][y];
    }
};

CircleData createCircle(int x, int y, int z, std::mt19937 gen) {
    CircleData data;

    //random radius for circles
    std::uniform_int_distribution<int> radiusDist(5, 50);
    int radius = radiusDist(gen);
    data.circle.setRadius(static_cast<float>(radius));

    data.circle.setPosition(static_cast<float>(x), static_cast<float>(y));

    //random generate for colours
    std::uniform_int_distribution<int> colorDist(0, 255);
    int colour[3];
    colour[0] = colorDist(gen);
    colour[1] = colorDist(gen);
    colour[2] = colorDist(gen);
    int opacity = colorDist(gen);
    data.circle.setFillColor(sf::Color(colour[0], colour[1], colour[2], opacity));

    data.depth = z;

    return data;
}

bool pixelInCircle(const sf::CircleShape& circle, int xPos, int yPos) {
    double distance = sqrt(pow(xPos - circle.getPosition().x - circle.getRadius(), 2) + pow(yPos - circle.getPosition().y - circle.getRadius(), 2));
    if(distance - 0.5 <= circle.getRadius())
        return true;
    else
        return false;
}

// Color2 is on the bottom(src) and Color1 is on the top(dst)
sf::Color blendColorsBitmaps(const sf::Color& color2, const sf::Color& color1) {
    float alpha1 = color1.a / 255.0f;
    float alpha2 = color2.a / 255.0f;
    float red1 = color1.r / 255.0f;
    float red2 = color2.r / 255.0f;
    float green1 = color1.g / 255.0f;
    float green2 = color2.g / 255.0f;
    float blue1 = color1.b / 255.0f;
    float blue2 = color2.b / 255.0f;

    float alphaFinal = alpha1 + alpha2 * (1.0f - alpha1);

    float redFinal   = (red1 * alpha1 + red2 * alpha2 * (1.0f - alpha1)) / alphaFinal;
    float greenFinal = (green1 * alpha1 + green2 * alpha2 * (1.0f - alpha1)) / alphaFinal;
    float blueFinal  = (blue1 * alpha1 + blue2 * alpha2 * (1.0f - alpha1)) / alphaFinal;

    sf::Color resultColor;
    resultColor.r = static_cast<sf::Uint8>(redFinal * 255.0f);
    resultColor.g = static_cast<sf::Uint8>(greenFinal * 255.0f);
    resultColor.b = static_cast<sf::Uint8>(blueFinal * 255.0f);
    resultColor.a = static_cast<sf::Uint8>(alphaFinal * 255.0f);
    return resultColor;
}

sf::Image bitmapToImage(BitMap& bitmap) {
    sf::Image image;
    image.create(bitmap.width, bitmap.height);

    for (int y = 0; y < bitmap.height; ++y) {
        for (int x = 0; x < bitmap.width; ++x) {
            PixelData pixel = bitmap.getPixel(x, y);
            image.setPixel(x, y, sf::Color(pixel.pixelColour.r, pixel.pixelColour.g,
                                           pixel.pixelColour.b, pixel.pixelColour.a));
        }
    }

    return image;
}

int main() {
    std::vector<float> parTimes;
    std::vector<float> seqTimes;
    std::vector<float> speedup;

    int maxDepth = 8;

    int imgWidth = 700;
    int imgHeight = 700;
    int offsetCenter = 20;

    //the same random device for all random numbers
    std::random_device rd;
    std::mt19937 gen(rd());

    // randomizer for circle center position
    std::uniform_int_distribution<int> posXDist(0, imgWidth - 2 * offsetCenter);
    std::uniform_int_distribution<int> posYDist(0, imgHeight - 2 * offsetCenter);
    std::uniform_int_distribution<int> posZDist(0, maxDepth);

    std::vector<int> shapeCounts = {1, 5, 10, 25, 50, 100, 250, 500, 750, 1000};

    for (int numShapes : shapeCounts) {
        int numberOfShapes = numShapes;
        std::map<int, std::vector<CircleData>> depthMap;
        bool displayTime = true;

        for (int i = 0; i < numberOfShapes; i++) {
            int xPosition = posXDist(gen);
            int yPosition = posYDist(gen);
            int zPosition = posZDist(gen);
            CircleData circleData = createCircle(xPosition, yPosition, zPosition, gen);
            depthMap[zPosition].push_back(circleData);
        }

        std::cout << "Start of sequential execution." << std::endl;
        sf::Clock clockSeq;

        sf::RenderWindow windowSeq(sf::VideoMode(imgWidth,
                                                 imgHeight), "Renderer Sequential");
        for (auto it = depthMap.rbegin(); it != depthMap.rend(); ++it) {
            // sorting the circles by depth
            std::sort(it->second.begin(), it->second.end(),
                      [](const CircleData &a, const CircleData &b) {
                          return a.depth > b.depth;
                      });

            for (const CircleData &circle: it->second) {
                windowSeq.draw(circle.circle);
            }
        }

        sf::Texture texture;
        texture.create(imgWidth, imgHeight);
        texture.update(windowSeq);
        sf::Image screenshot = texture.copyToImage();
        screenshot.saveToFile("screenSeq_" + std::to_string(numberOfShapes) + "Shapes.jpg");


        while (windowSeq.isOpen()) {
            sf::Event event;
            while (windowSeq.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    windowSeq.close();
                }
            }
            windowSeq.clear();

            sf::Sprite sprite(texture);
            windowSeq.draw(sprite);
            windowSeq.display();

            if (displayTime) {
                sf::Time elapsed = clockSeq.getElapsedTime();
                //std::cout << "Elapsed time: " << elapsed.asSeconds() << " seconds for " << numberOfShapes << " shapes." << std::endl;
                seqTimes.push_back(elapsed.asSeconds());
                displayTime = false;
            }
        }


        std::cout << "Start of parallel execution with OpenMP." << std::endl;
        displayTime = true;
        sf::Clock clockPar;
        std::map<int, BitMap> bitmapsWithDepth;

#pragma omp parallel for
        for (int i = 0; i <= maxDepth; ++i) {
            bitmapsWithDepth.emplace(i, BitMap(imgWidth, imgHeight));
        }
        BitMap bitmapFinal(imgWidth, imgHeight);

#pragma omp for
        for (int depth = 0; depth <= maxDepth; ++depth) {
            auto it = depthMap.find(depth);
            if (it != depthMap.end()) {
                const std::vector<CircleData> &circles = it->second;
                BitMap &bitmapTmp = bitmapsWithDepth[depth];

                for (const CircleData &circle: circles) {
                    float radius = circle.circle.getRadius();
                    float centerX = circle.circle.getPosition().x + radius;
                    float centerY = circle.circle.getPosition().y + radius;

                    for (int j = static_cast<int>(centerY - radius); j <= static_cast<int>(centerY + radius); ++j) {
                        for (int i = static_cast<int>(centerX - radius);
                             i <= static_cast<int>(centerX + radius); ++i) {
                            // check if the pixel is inside the circle and within the window bounds
                            if (pixelInCircle(circle.circle, i, j) && i >= 0 && i < imgWidth && j >= 0 &&
                                j < imgHeight) {
                                // get current bitmap's pixel colour
                                sf::Color colorTmp = bitmapTmp.getPixel(i, j).pixelColour;

                                // if no previous color exists write it, otherwise blend colors
                                if ((int) colorTmp.r == 0 && (int) colorTmp.g == 0 &&
                                    (int) colorTmp.b == 0 && (int) colorTmp.a == 0) {
                                    bitmapTmp.getPixel(i, j).pixelColour = circle.circle.getFillColor();
                                } else {
                                    bitmapTmp.getPixel(i, j).pixelColour =
                                            blendColorsBitmaps(colorTmp, circle.circle.getFillColor());
                                }
                            }
                        }
                    }
                }
            }
        }

#pragma omp for
        for (int j = 0; j < imgHeight; ++j) {
            for (int i = 0; i < imgWidth; ++i) {
                sf::Color localFinalColor = bitmapFinal.getPixel(i, j).pixelColour;
                for (auto it = bitmapsWithDepth.rbegin(); it != bitmapsWithDepth.rend(); ++it) {
                    sf::Color colorTmp = it->second.getPixel(i, j).pixelColour;

                    // base layer blank
                    if ((int)localFinalColor.r == 0 && (int)localFinalColor.g == 0 &&
                        (int)localFinalColor.b == 0 && (int)localFinalColor.a == 0) {
                        localFinalColor = colorTmp;
                    }
                    // pixel has colour
                    else if (colorTmp.a > 0) {
                        localFinalColor = blendColorsBitmaps(localFinalColor, colorTmp);
                    }
                }

#pragma omp critical
                {
                    bitmapFinal.getPixel(i, j).pixelColour = localFinalColor;
                }
            }
        }
        sf::Image image = bitmapToImage(bitmapFinal);
        sf::RenderWindow windowPar(sf::VideoMode(imgWidth, imgHeight), "Renderer Parallel");

        while (windowPar.isOpen()) {
            sf::Event event;
            while (windowPar.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    // Save the final rendered image to a file
                    std::ostringstream filenameStream;
                    filenameStream << "screenPar_" << numberOfShapes << "Shapes.jpg";
                    std::string filename = filenameStream.str();
                    sf::Texture texture;
                    texture.create(windowPar.getSize().x, windowPar.getSize().y);
                    texture.update(windowPar);
                    sf::Image screenshot = texture.copyToImage();
                    screenshot.saveToFile(filename);
                    windowPar.close();
                }
            }

            windowPar.clear();
            sf::Texture texture;
            texture.loadFromImage(image);
            sf::Sprite sprite(texture);
            windowPar.draw(sprite);
            windowPar.display();
            if (displayTime) {
                sf::Time elapsed = clockPar.getElapsedTime();
                //std::cout << "Elapsed time: " << elapsed.asSeconds() << " seconds for " << numberOfShapes << " shapes." << std::endl;
                parTimes.push_back(elapsed.asSeconds());
                displayTime = false;
            }
        }
        std::cout << "- - - - - - - - - - - - - - - -" << std::endl;
    }

    // speed up calculator
    for (size_t i = 0; i < seqTimes.size(); ++i) {
        float result = seqTimes[i] / parTimes[i];
        speedup.push_back(result);
    }
    std::cout << "Result vector for speedup: ";
    for (float i: speedup) {
        std::cout << i << " ";
    }
    std::cout << std::endl;

    return 0;
}
