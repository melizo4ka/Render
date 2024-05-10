#include <algorithm>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <random>
#include <sstream>
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
        pixelColour.a = 1;
    }
};

struct BitMap {
    int width;
    int height;
    std::vector<std::vector<PixelData>> pixels;

    BitMap(int w, int h) : width(w), height(h) {
        pixels.resize(height, std::vector<PixelData>(width));
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
    double distance = sqrt(pow(xPos - circle.getPosition().x, 2) + pow(yPos - circle.getPosition().y, 2));
    if(distance <= circle.getRadius())
        return true;
    else
        return false;
}

// if Color1 is on the bottom and Color2 is on the top
sf::Color blendColors(const sf::Color& color2, int opacity2, const sf::Color& color1, int opacity1) {
    float alpha1 = static_cast<float>(opacity1) / 255.0f;
    float alpha2 = static_cast<float>(opacity2) / 255.0f;
    float resultAlpha = alpha1 + alpha2 * (1 - alpha1);
    sf::Color resultColor;
    resultColor.r = static_cast<sf::Uint8>((color1.r * alpha1) + (color2.r * alpha2 * (1 - alpha1)));
    resultColor.g = static_cast<sf::Uint8>((color1.g * alpha1) + (color2.g * alpha2 * (1 - alpha1)));
    resultColor.b = static_cast<sf::Uint8>((color1.b * alpha1) + (color2.b * alpha2 * (1 - alpha1)));
    //resultColor.a = static_cast<sf::Uint8>(resultAlpha * 255);

    return resultColor;
}

sf::Color calculateRGBA (const std::vector<circleInfluence>& influences){
    sf::Color result;
    sf::Color inter_result;
    result = blendColors( influences[0].infColour, influences[0].infColour.a,
                          influences[1].infColour, influences[1].infColour.a);
    if (influences.size() == 2)
        return result;
    else{
        for (int k = 2; k < influences.size(); k++){
            inter_result = blendColors( result, result.a,
                                        influences[k].infColour, influences[k].infColour.a);
            result = inter_result;
        }
        return result;
    }
}

sf::Image bitmapToImage(BitMap& bitmap) {
    sf::Image image;
    image.create(bitmap.width, bitmap.height);

    for (int y = 0; y < bitmap.height; ++y) {
        for (int x = 0; x < bitmap.width; ++x) {
            PixelData pixel = bitmap.getPixel(x, y);
            image.setPixel(x, y, sf::Color(pixel.pixelColour.r, pixel.pixelColour.g, pixel.pixelColour.b, pixel.pixelColour.a));
        }
    }

    return image;
}

int main() {

    std::vector<float> parTimes;
    std::vector<float> seqTimes;
    std::vector<float> speedup;

    std::vector<int> differentNoS = {5};
    int maxDepth = 20;

    int imgWidth = 1000;
    int imgHeight = 1000;

    //the same random device for all random numbers
    std::random_device rd;
    std::mt19937 gen(rd());

    // randomizer for circle center position
    std::uniform_int_distribution<int> posXDist(0, imgWidth);
    std::uniform_int_distribution<int> posYDist(0, imgHeight);
    std::uniform_int_distribution<int> posZDist(0, maxDepth);

    int numberOfShapes = differentNoS[0];
    CircleData shapes[numberOfShapes];

    // TODO creating of shapes separately

    std::cout << "Start of sequential execution." << std::endl;
    for (int numberOfShapes : differentNoS){
        bool displayTime = true;
        sf::Clock clock;

        for (int i = 0; i < numberOfShapes; i++) {
            int xPosition = posXDist(gen);
            int yPosition = posYDist(gen);
            int zPosition = posZDist(gen);

            shapes[i] = createCircle(xPosition, yPosition, zPosition, gen);
        }

        // Sort the array based on depth, to draw from back to front
        std::sort(shapes, shapes + numberOfShapes, [](const CircleData& a, const CircleData& b) {
            return a.depth > b.depth;
        });

        sf::RenderWindow window(sf::VideoMode(imgWidth,
                                              imgHeight),"Renderer Sequential");

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed){
                    std::ostringstream filenameStream;
                    filenameStream << "screenSeq_" << numberOfShapes << "Shapes.jpg";
                    std::string filename = filenameStream.str();
                    sf::Texture texture;
                    texture.create(window.getSize().x, window.getSize().y);
                    texture.update(window);
                    sf::Image screenshot = texture.copyToImage();
                    screenshot.saveToFile(filename);
                    window.close();
                }
            }
            window.clear();

            for (int i = 0; i < numberOfShapes; i++) {
                window.draw(shapes[i].circle);
            }
            window.display();

            if (displayTime) {
                sf::Time elapsed = clock.getElapsedTime();
                // std::cout << "Elapsed time: " << elapsed.asSeconds() << " seconds for " << numberOfShapes << " shapes." << std::endl;
                seqTimes.push_back(elapsed.asSeconds());
                displayTime = false;
            }

        }

    }

    std::cout << std::endl;


    std::cout << "Start of parallel execution." << std::endl;
    bool displayTime = true;

# pragma omp parallel for shared(differentNoS, imgHeight, imgWidth, shapes) default (none)

    for (int numberOfShapes : differentNoS) {
        printf("Maximum number of threads after: %d\n", omp_get_num_threads());

        BitMap bitmap(imgWidth, imgHeight);

        //iterations to create the bitmap pixel by pixel
        for (int j = 0; j < imgHeight; j++) {
            for (int i = 0; i < imgWidth; i++) {
                //creating new data structure
                std::vector<circleInfluence> influences;
                // iterate over all the circles
                for (int n = 0; n < numberOfShapes; n++) {
                    // call checker
                    if (pixelInCircle(shapes[n].circle, i, j)) {
                        // if the condition is true then we add info of that circle in the data structure
                        influences.emplace_back(shapes[n].depth, shapes[n].circle.getFillColor());
                    }
                }

                // this part of the code is not used for now since shapes is already ordered in the sequential section
                /*if (!influences.empty()) {
                    if (influences.size()> 1) {
                        std::sort(influences.begin(), influences.end(),
                                  [](const circleInfluence &a, const circleInfluence &b) {
                                      return a.depth > b.depth;
                                  });
                    }
                }*/

                if (influences.size() > 1) {
                    sf::Color finalColor = calculateRGBA(influences);
                    bitmap.getPixel(i, j).pixelColour = finalColor;
                } else {
                    // just put values into bitmap
                    if (!influences.empty())
                        bitmap.getPixel(i, j).pixelColour = influences[0].infColour;
                }
            }
        }

        sf::Image image = bitmapToImage(bitmap);
        sf::RenderWindow window(sf::VideoMode(imgWidth,
                                              imgHeight), "Renderer Parallel");

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {

                    std::ostringstream filenameStream;
                    filenameStream << "screenPar_" << numberOfShapes << "Shapes.jpg";
                    std::string filename = filenameStream.str();
                    sf::Texture texture;
                    texture.create(window.getSize().x, window.getSize().y);
                    texture.update(window);
                    sf::Image screenshot = texture.copyToImage();
                    screenshot.saveToFile(filename);
                    window.close();
                }
            }
            window.clear();
            sf::Texture texture;
            texture.loadFromImage(image);
            sf::Sprite sprite(texture);
            window.draw(sprite);
            window.display();
        }
    }

//SPEED UP CALCULATOR
/*
    for (size_t i = 0; i < seqTimes.size(); ++i) {
        double result = seqTimes[i] / parTimes[i];
        speedup.push_back(result);
    }
    // Print the result vector
    std::cout << "Result vector for speedup: ";
    for (const auto &element : speedup) {
        std::cout << element << " ";
    }
    std::cout << std::endl;

*/

    return 0;
}
