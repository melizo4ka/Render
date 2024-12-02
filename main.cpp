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

struct circleInfluence {
    sf::Color infColour;
    int depth;

    circleInfluence(int dp, const sf::Color& fillColor) : depth(dp) {
        infColour.r = fillColor.r;
        infColour.g = fillColor.g;
        infColour.b = fillColor.b;
        infColour.a = fillColor.a;
    }
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

sf::Color blendColors(const sf::Color& color2, const sf::Color& color1) {
    float alpha1 = static_cast<float>(color1.a) / 255.0f;
    float alpha2 = static_cast<float>(color2.a) / 255.0f;

    sf::Color resultColor;
    resultColor.r = static_cast<sf::Uint8>((color1.r + color2.r) / 2);
    resultColor.g = static_cast<sf::Uint8>((color1.g + color2.g) / 2);
    resultColor.b = static_cast<sf::Uint8>((color1.b + color2.b) / 2);
    //resultColor.a = static_cast<sf::Uint8>(((alpha1+ alpha2) / 2) * 255.0f);

    return resultColor;
}

// if Color2 is on the bottom and Color1 is on the top
sf::Color blendColorsBitmaps(const sf::Color& color2, const sf::Color& color1) {
    float alpha1 = static_cast<float>(color1.a) / 255.0f;
    float alpha2 = static_cast<float>(color2.a) / 255.0f;

    sf::Color resultColor;
    resultColor.r = static_cast<sf::Uint8>((color1.r * alpha1) + (color2.r * alpha2 * (1 - alpha1)));
    resultColor.g = static_cast<sf::Uint8>((color1.g * alpha1) + (color2.g * alpha2 * (1 - alpha1)));
    resultColor.b = static_cast<sf::Uint8>((color1.b * alpha1) + (color2.b * alpha2 * (1 - alpha1)));

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

    int maxDepth = 20;

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

    int numberOfShapes = 1000;
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
                  [](const CircleData& a, const CircleData& b) {
                      return a.depth > b.depth;
                  });

        for (const CircleData& circle : it->second) {
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
            std::cout << "Elapsed time: " << elapsed.asSeconds() << " seconds for " << numberOfShapes << " shapes."
                      << std::endl;
            seqTimes.push_back(elapsed.asSeconds());
            displayTime = false;
        }
    }

        /*
        std::cout << "Start of parallel execution (Second version)." << std::endl;
        displayTime = true;
        sf::Clock clockPar;

        const int numBitmaps = maxDepth + 1;
        std::map<int, BitMap> bitmapsWithDepth;

        for (int i = 0; i <= numBitmaps; ++i) {
            int depth = i;
            BitMap bitmap(imgWidth, imgHeight);
            bitmapsWithDepth.emplace(depth, std::move(bitmap));
        }

        BitMap bitmapFinal(imgWidth, imgHeight);

        //#pragma omp parallel for default(none) shared(bitmapsWithDepth, shapes, numberOfShapes, imgWidth, imgHeight)
        for (int n = 0; n < numberOfShapes; n++) {
            float radius = shapes[n].circle.getRadius();
            float centerX = shapes[n].circle.getPosition().x + radius;
            float centerY = shapes[n].circle.getPosition().y + radius;
            for (int j = static_cast<int>(centerY - radius); j <= static_cast<int>(centerY + radius); j++) {
                for (int i = static_cast<int>(centerX - radius); i <= static_cast<int>(centerX + radius); i++){
                    // need to check if that particular pixel is inside the circle and if the pixel is inside the window
                    if (pixelInCircle(shapes[n].circle, i, j) && i >= 0 && i < imgWidth &&
                            j >= 0 && j <= imgHeight) {

                        BitMap& bitmapTmp = bitmapsWithDepth[shapes[n].depth];
                        sf::Color colorTmp = bitmapTmp.getPixel(i, j).pixelColour;
                        // check if there is already a colour written in that pixel
                        if((int)colorTmp.r == 0 && (int)colorTmp.g == 0 && (int)colorTmp.b == 0 && (int)colorTmp.a == 255)
                            bitmapTmp.getPixel(i, j).pixelColour = shapes[n].circle.getFillColor();
                        else
                            // do the blending
                            bitmapTmp.getPixel(i, j).pixelColour = blendColorsBitmaps(colorTmp, shapes[n].circle.getFillColor());
                    }
                }
            }
        }

        //#pragma omp for collapse(2)
        for (int j = 0; j < imgHeight; ++j) {
            for (int i = 0; i < imgWidth; ++i) {
                // Iterate through all bitmaps (corresponding to different depths)
                for (int k = 0; k >= 0; k--){
                //for (int k = numBitmaps; k >= 0; k--) {
                    sf::Color colorTmp = bitmapsWithDepth[k].getPixel(i, j).pixelColour;
                    if ((int)colorTmp.r != 0 || (int)colorTmp.g != 0 || (int)colorTmp.b != 0 || (int)colorTmp.a != 255) {
                        // blend colors
                        bitmapFinal.getPixel(i, j).pixelColour =
                                blendColorsBitmaps(bitmapFinal.getPixel(i, j).pixelColour,colorTmp);
                    }
                }
            }
        }

        sf::Image image = bitmapToImage(bitmapFinal);
        sf::RenderWindow windowPar(sf::VideoMode(imgWidth,
                                                 imgHeight), "Renderer Parallel");

        if (displayTime) {
            sf::Time elapsed = clockPar.getElapsedTime();
            std::cout << "Elapsed time: " << elapsed.asSeconds() << " seconds for " << numberOfShapes << " shapes."
                      << std::endl;
            parTimes.push_back(elapsed.asSeconds());
            displayTime = false;
        }

        while (windowPar.isOpen()) {
            sf::Event event;
            while (windowPar.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {

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

        }


    }
         */
    std::cout << "Start of parallel execution with OpenMP. Third Version." << std::endl;
    displayTime = true;
    sf::Clock clockPar;
    std::map<int, BitMap> bitmapsWithDepth;
    const int numBitmaps = maxDepth + 1;

    for (int i = 0; i <= maxDepth; ++i) {
        bitmapsWithDepth.emplace(i, BitMap(imgWidth, imgHeight));
    }

    BitMap bitmapFinal(imgWidth, imgHeight);

#pragma omp parallel default(none) shared(depthMap, bitmapsWithDepth, imgWidth, imgHeight, maxDepth)
{
#pragma omp for
    for (int depth = 0; depth <= maxDepth; ++depth) {
        auto it = depthMap.find(depth);
        if (it != depthMap.end()) {
            const std::vector<CircleData>& circles = it->second;
            BitMap& bitmapTmp = bitmapsWithDepth[depth];

            for (const CircleData& circle : circles) {
                float radius = circle.circle.getRadius();
                float centerX = circle.circle.getPosition().x + radius;
                float centerY = circle.circle.getPosition().y + radius;

                for (int j = static_cast<int>(centerY - radius); j <= static_cast<int>(centerY + radius); ++j) {
                    for (int i = static_cast<int>(centerX - radius); i <= static_cast<int>(centerX + radius); ++i) {
                        // Check if the pixel is inside the circle and within the window bounds
                        if (pixelInCircle(circle.circle, i, j) && i >= 0 && i < imgWidth && j >= 0 && j < imgHeight) {
                            // get current bitmap's pixel colour
                            sf::Color colorTmp = bitmapTmp.getPixel(i, j).pixelColour;

                            // Only write if no previous color exists, otherwise blend colors
                            if ((int)colorTmp.r == 0 && (int)colorTmp.g == 0 &&
                                (int)colorTmp.b == 0 && (int)colorTmp.a == 0) {
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
}
    //bitmapFinal = bitmapsWithDepth[9];
    for (int j = 0; j < imgHeight; ++j) {
        for (int i = 0; i < imgWidth; ++i) {
            for (auto it = bitmapsWithDepth.rbegin(); it != bitmapsWithDepth.rend(); ++it) {
                sf::Color colorTmp = it->second.getPixel(i, j).pixelColour;
                sf::Color colorFinalTmp = bitmapFinal.getPixel(i, j).pixelColour;
                // base layer blank
                if ((int)colorFinalTmp.r == 0 && (int)colorFinalTmp.g == 0 &&
                    (int)colorFinalTmp.b == 0 && (int)colorFinalTmp.a == 0){
                    bitmapFinal.getPixel(i, j).pixelColour = colorTmp;
                }
                // pixel has colour
                else if (colorTmp.a > 0) {
                    bitmapFinal.getPixel(i, j).pixelColour =
                            blendColorsBitmaps(bitmapFinal.getPixel(i, j).pixelColour, colorTmp);
                }
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
            std::cout << "Elapsed time: " << elapsed.asSeconds() << " seconds for " << numberOfShapes << " shapes."
                      << std::endl;
            parTimes.push_back(elapsed.asSeconds());
            displayTime = false;
        }
    }



//SPEED UP CALCULATOR
/*
    for (size_t i = 0; i < seqTimes.size(); ++i) {
        float result = seqTimes[i] / parTimes[i];
        speedup.push_back(result);
    }
    // Print the result vector
    std::cout << "Result vector for speedup: ";
    for (float i : speedup) {
        std::cout << i << " ";
    }
    std::cout << std::endl;*/

    return 0;
}
