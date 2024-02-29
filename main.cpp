#include <algorithm>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <random>
#include <omp.h>
#include <X11/Xlib.h>

struct CircleData {
    sf::CircleShape circle;
    int depth;
};

CircleData createCircle(int x, int y, int z, int opacity) {
    CircleData data;

    //random radius for circles
    std::random_device rdr;
    std::mt19937 genRad(rdr());
    std::uniform_int_distribution<int> radiusDist(5, 50);
    int radius = radiusDist(genRad);
    data.circle.setRadius(static_cast<float>(radius));

    data.circle.setPosition(static_cast<float>(x), static_cast<float>(y));
    //random generate for colours
    std::random_device rdc;
    std::mt19937 genCol(rdc());
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
    // Initialize Xlib for multi-threading
    XInitThreads();
    //checking of num of processors
#ifdef _OPENMP
    std::cout << "_OPENMP defined" << std::endl;
    std::cout << "Num processors (Phys+HT): " << omp_get_num_procs() << std::endl;
#endif

    // Set a flag to decide whether to execute the parallel loop or not
    bool executeParallelLoop = true;
    if (executeParallelLoop)
        std::cout << "This is parallel execution." << std::endl;
    else
        std::cout << "This is sequential execution." << std::endl;
    int differentNoS[] = {10, 100, 500, 1000, 10000, 20000};

    for (int numberOfShapes : differentNoS){
        CircleData shapes[numberOfShapes];

        //variable to get time til image is displayed
        bool displayTime = true;
        sf::Clock clock;

        //random generate for colours
        std::random_device rdp;
        std::mt19937 genPos(rdp());
        std::uniform_int_distribution<int> posXDist(0, sf::VideoMode::getDesktopMode().width - 150);
        std::uniform_int_distribution<int> posYDist(0, sf::VideoMode::getDesktopMode().height - 150);
        std::uniform_int_distribution<int> posZDist(0, 50);

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
                                              sf::VideoMode::getDesktopMode().height), "Renderer");

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed){
                    //to save window image produced, file is saved to bin folder
                    sf::Texture texture;
                    texture.create(window.getSize().x, window.getSize().y);
                    texture.update(window);
                    sf::Image screenshot = texture.copyToImage();
                    screenshot.saveToFile("screenshot.jpg");
                    window.close();
                }
            }

            //white background for aesthetic purposes
            window.clear(sf::Color::White);

            if (executeParallelLoop) {
#pragma omp for
                //drawing of circles from ordered array
                for (int i = 0; i < numberOfShapes; i++) {
                    window.draw(shapes[i].circle);
                }
            }
            else{
                //drawing of circles from ordered array
                for (int i = 0; i < numberOfShapes; i++) {
                    window.draw(shapes[i].circle);
                }
            }

            window.display();

            //stop counting time here
            if (displayTime) {
                sf::Time elapsed = clock.getElapsedTime();
                std::cout << "Elapsed time: " << elapsed.asSeconds() << " seconds for " << numberOfShapes << " shapes." << std::endl;
                displayTime = false;
            }

        }

    }

    return 0;
}
