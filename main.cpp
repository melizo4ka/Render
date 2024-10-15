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
        pixelColour.a = 255;
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
    resultColor.a = static_cast<sf::Uint8>(((alpha1+ alpha2) / 2) * 255.0f);

    return resultColor;
}

// if Color2 is on the bottom and Color1 is on the top
sf::Color blendColorsBitmaps(const sf::Color& color2, const sf::Color& color1) {
    float alpha1 = static_cast<float>(color1.a) / 255.0f;
    float alpha2 = static_cast<float>(color2.a) / 255.0f;
    float resultAlpha = alpha1 + alpha2 * (1 - alpha1);

    sf::Color resultColor;
    resultColor.r = static_cast<sf::Uint8>((color1.r * alpha1) + (color2.r * alpha2 * (1 - alpha1)));
    resultColor.g = static_cast<sf::Uint8>((color1.g * alpha1) + (color2.g * alpha2 * (1 - alpha1)));
    resultColor.b = static_cast<sf::Uint8>((color1.b * alpha1) + (color2.b * alpha2 * (1 - alpha1)));
    resultColor.a = static_cast<sf::Uint8>(resultAlpha * 255.0f);

    return resultColor;
}

sf::Color calculateRGBA (const std::vector<circleInfluence>& influences){
    sf::Color result;
    sf::Color inter_result;
    result = blendColors( influences[0].infColour, influences[1].infColour);
    if (influences.size() == 2)
        return result;
    else{
        for (int k = 2; k < influences.size(); k++){
            inter_result = blendColors( result,influences[k].infColour);
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

    std::vector<int> differentNoS = {2000};


    for (int numberOfShapes: differentNoS) {
        bool displayTime = true;
        CircleData shapes[numberOfShapes];

        // creation of cicles, not accounting that into the program's time
        for (int i = 0; i < numberOfShapes; i++) {
            int xPosition = posXDist(gen);
            int yPosition = posYDist(gen);
            int zPosition = posZDist(gen);
            shapes[i] = createCircle(xPosition, yPosition, zPosition, gen);
        }

        std::cout << "Start of sequential execution." << std::endl;
        sf::Clock clockSeq;

        // Sort the array based on depth, to draw from back to front
        std::sort(shapes, shapes + numberOfShapes, [](const CircleData &a, const CircleData &b) {
            return a.depth > b.depth;
        });

        sf::RenderWindow windowSeq(sf::VideoMode(imgWidth,
                                                 imgHeight), "Renderer Sequential");

        if (displayTime) {
            sf::Time elapsed = clockSeq.getElapsedTime();
            std::cout << "Elapsed time: " << elapsed.asSeconds() << " seconds for " << numberOfShapes << " shapes."
                      << std::endl;
            seqTimes.push_back(elapsed.asSeconds());
            displayTime = false;
        }

        while (windowSeq.isOpen()) {
            sf::Event event;
            while (windowSeq.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    std::ostringstream filenameStream;
                    filenameStream << "screenSeq_" << numberOfShapes << "Shapes.jpg";
                    std::string filename = filenameStream.str();
                    sf::Texture texture;
                    texture.create(windowSeq.getSize().x, windowSeq.getSize().y);
                    texture.update(windowSeq);
                    sf::Image screenshot = texture.copyToImage();
                    screenshot.saveToFile(filename);
                    windowSeq.close();
                }
            }
            windowSeq.clear();

            for (int i = 0; i < numberOfShapes; i++) {
                //if(shapes[i].depth == 0)
                windowSeq.draw(shapes[i].circle);
            }
            windowSeq.display();


        }
/*
        std::cout << "Start of parallel execution (First Version)." << std::endl;
        displayTime = true;
        sf::Clock clockPar;

        //iterations to create the bitmap pixel by pixel
        BitMap bitmap(imgWidth, imgHeight);

        omp_set_num_threads(2);
#pragma omp parallel default(none) shared(bitmap, shapes, numberOfShapes, imgWidth, imgHeight, cout)
        {
            //#pragma omp single
            //std::cout << "Number of threads: " << omp_get_num_threads() << std::endl;

#pragma omp for collapse(2) schedule(dynamic, 1) nowait
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
                    if (!influences.empty()) {
                        if (influences.size() > 1) {
                            std::sort(influences.begin(), influences.end(),
                                      [](const circleInfluence &a, const circleInfluence &b) {
                                          return a.depth > b.depth;
                                      });

                        }
                    }
                    sf::Color finalColor;
                    if (influences.size() > 1) {
                        finalColor = calculateRGBA(influences);
                        //bitmap.getPixel(i, j).pixelColour = finalColor;
                    } else {
                        // just put values into bitmap
                        if (!influences.empty())
                            finalColor = influences[0].infColour;
                        //bitmap.getPixel(i, j).pixelColour = influences[0].infColour;
                    }
#pragma omp atomic write
                    bitmap.getPixel(i, j).pixelColour.r = finalColor.r;

#pragma omp atomic write
                    bitmap.getPixel(i, j).pixelColour.g = finalColor.g;

#pragma omp atomic write
                    bitmap.getPixel(i, j).pixelColour.b = finalColor.b;

#pragma omp atomic write
                    bitmap.getPixel(i, j).pixelColour.a = finalColor.a;
                }
            }
        }
        sf::Image image = bitmapToImage(bitmap);
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

*/
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
                        //TODO add atomic for writing
                        if((int)colorTmp.r == 0 && (int)colorTmp.g == 0 && (int)colorTmp.b == 0 && (int)colorTmp.a == 255)
                            bitmapTmp.getPixel(i, j).pixelColour = shapes[n].circle.getFillColor();
                        else
                            // do the blending
                            bitmapTmp.getPixel(i, j).pixelColour = blendColors(colorTmp, shapes[n].circle.getFillColor());
                    }
                }
            }
        }

        //#pragma omp for collapse(2)
        for (int j = 0; j < imgHeight; ++j) {
            for (int i = 0; i < imgWidth; ++i) {
                // Iterate through all bitmaps (corresponding to different depths)
                //for (int k = 0; k >= 0; k--){
                for (int k = numBitmaps; k >= 0; k--) {
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
