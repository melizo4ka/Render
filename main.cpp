#include <algorithm>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <random>
#include <sstream>


struct CircleData {
    sf::CircleShape circle;
    int depth;
};

CircleData createCircle(int x, int y, int z, int opacity) {
    CircleData data;

    //random radius for circles
    std::random_device rd;
    std::mt19937 genRad(rd());
    std::uniform_int_distribution<int> radiusDist(5, 50);
    int radius = radiusDist(genRad);
    data.circle.setRadius(static_cast<float>(radius));

    data.circle.setPosition(static_cast<float>(x), static_cast<float>(y));
    //random generate for colours
    std::mt19937 genCol(rd());
    std::uniform_int_distribution<int> colorDist(0, 255);
    int colour[3];
    colour[0] = colorDist(genCol);
    colour[1] = colorDist(genCol);
    colour[2] = colorDist(genCol);

    data.circle.setFillColor(sf::Color(colour[0], colour[1], colour[2], opacity));
    data.depth = z;

    return data;
}

int main() {
    std::vector<float> parTimes;
    std::vector<float> seqTimes;
    std::vector<float> speedup;
    // first value should be discarded
    int differentNoS[] = {0, 10, 100, 500, 1000, 10000, 20000};

    // parallel section
    bool executeParallelLoop = true;
    std::cout << "Start of parallel execution." << std::endl;

    for (int numberOfShapes : differentNoS){
        CircleData shapes[numberOfShapes];

        //random generate for position
        std::random_device rdp;
        std::mt19937 genPos(rdp());
        std::uniform_int_distribution<int> posXDist(0, sf::VideoMode::getDesktopMode().width - 150);
        std::uniform_int_distribution<int> posYDist(0, sf::VideoMode::getDesktopMode().height - 150);
        std::uniform_int_distribution<int> posZDist(0, 50);

        //variable to get time til image is displayed
        bool displayTime = true;
        sf::Clock clock;

#pragma omp parallel for if(executeParallelLoop)
        for (int i = 0; i < numberOfShapes; i++) {
            int xPosition = posXDist(genPos);
            int yPosition = posYDist(genPos);
            int zPosition = posZDist(genPos);
            int opacity = 50;

            //creating shaped one at a time and storing into array
            shapes[i] = createCircle(xPosition, yPosition, zPosition, opacity);
        }

        // Sort the array based on depth, to draw from back to front
        std::sort(shapes, shapes + numberOfShapes, [](const CircleData& a, const CircleData& b) {
            return a.depth > b.depth;
        });


        sf::RenderWindow window(sf::VideoMode(sf::VideoMode::getDesktopMode().width,
                                              sf::VideoMode::getDesktopMode().height), "Renderer Parallel");

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed){
                    std::ostringstream filenameStream;
                    filenameStream << "screenshotPar_" << numberOfShapes << "Shapes.jpg";
                    std::string filename = filenameStream.str();

                    //to save window image produced, file is saved to bin folder
                    sf::Texture texture;
                    texture.create(window.getSize().x, window.getSize().y);
                    texture.update(window);
                    sf::Image screenshot = texture.copyToImage();
                    screenshot.saveToFile(filename);
                    window.close();
                }
            }

            //white background for aesthetic purposes
            window.clear(sf::Color::White);

#pragma omp for
            for (int i = 0; i < numberOfShapes; i++) {
                window.draw(shapes[i].circle);
            }
            window.display();

            if (displayTime) {
                sf::Time elapsed = clock.getElapsedTime();
                // std::cout << "Elapsed time: " << elapsed.asSeconds() << " seconds for " << numberOfShapes << " shapes." << std::endl;
                parTimes.push_back(elapsed.asSeconds());
                displayTime = false;
            }
        }
    }

    std::cout << "Start of sequential execution." << std::endl;
    for (int numberOfShapes : differentNoS){
        CircleData shapes[numberOfShapes];

        std::random_device rdp;
        std::mt19937 genPos(rdp());
        std::uniform_int_distribution<int> posXDist(0, sf::VideoMode::getDesktopMode().width - 150);
        std::uniform_int_distribution<int> posYDist(0, sf::VideoMode::getDesktopMode().height - 150);
        std::uniform_int_distribution<int> posZDist(0, 50);

        bool displayTime = true;
        sf::Clock clock;

        for (int i = 0; i < numberOfShapes; i++) {
            int xPosition = posXDist(genPos);
            int yPosition = posYDist(genPos);
            int zPosition = posZDist(genPos);
            int opacity = 50;

            shapes[i] = createCircle(xPosition, yPosition, zPosition, opacity);
        }

        std::sort(shapes, shapes + numberOfShapes, [](const CircleData& a, const CircleData& b) {
            return a.depth > b.depth;
        });


        sf::RenderWindow window(sf::VideoMode(sf::VideoMode::getDesktopMode().width,
                                              sf::VideoMode::getDesktopMode().height),
                                "Renderer Sequential");

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed){
                    std::ostringstream filenameStream;
                    filenameStream << "screenshotSeq_" << numberOfShapes << "Shapes.jpg";
                    std::string filename = filenameStream.str();
                    sf::Texture texture;
                    texture.create(window.getSize().x, window.getSize().y);
                    texture.update(window);
                    sf::Image screenshot = texture.copyToImage();
                    screenshot.saveToFile(filename);
                    window.close();
                }
            }
            window.clear(sf::Color::White);

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

    return 0;
}
