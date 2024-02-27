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
    data.circle.setRadius(50.f);
    data.circle.setPosition(static_cast<float>(x), static_cast<float>(y));

    //random generate for colours
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> colorDist(0, 255);

    int colour[3];
    colour[0] = colorDist(gen);
    colour[1] = colorDist(gen);
    colour[2] = colorDist(gen);
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

    //more than a 1000 can crash on my PC
    int numberOfShapes = 1100;

    CircleData shapes[numberOfShapes];

    //variable to get time til image is displayed
    bool displayTime = true;
    sf::Clock clock;

#pragma omp parallel for
    for (int i = 0; i < numberOfShapes; i++) {
        //added condition of "-150" to have all circles drawn inside the window
        int xPosition = rand() % (sf::VideoMode::getDesktopMode().width - 150);
        int yPosition = rand() % (sf::VideoMode::getDesktopMode().height - 150);

        int zPosition = rand() % 100;
        int opacity = 50;

        //creating shaped one at a time and storing into array
        shapes[i] = createCircle(xPosition, yPosition, zPosition, opacity);
    }

    // Sort the array based on depth, to draw from back to front
#pragma omp parallel
    {
#pragma omp single
        {
            std::sort(shapes, shapes + numberOfShapes, [](const CircleData &a, const CircleData &b) {
                return a.depth > b.depth;
            });
        }
    }

    sf::RenderWindow window(sf::VideoMode(sf::VideoMode::getDesktopMode().width,
                                          sf::VideoMode::getDesktopMode().height), "Renderer");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
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

        /*
        //drawing of circles from ordered array
        for (int i = 0; i < numberOfShapes; i++) {
            window.draw(shapes[i].circle);
        }
*/
        //stop counting time here
        if (displayTime) {
            sf::Time elapsed = clock.getElapsedTime();
            std::cout << "Elapsed time: " << elapsed.asSeconds() << " seconds" << std::endl;
            displayTime = false;
        }

    }
    return 0;
}
