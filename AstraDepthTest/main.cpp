#include <SFML/Graphics.hpp>
#include <astra/astra.hpp>
#include "LitDepthVisualizer.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace astra;

DepthStream configure_depth(StreamReader& reader)
{
	auto depthStream = reader.stream<DepthStream>();

	auto oldMode = depthStream.mode();

	//We don't have to set the mode to start the stream, but if you want to here is how:
	ImageStreamMode depthMode;

	depthMode.set_width(640);
	depthMode.set_height(480);
	depthMode.set_pixel_format(astra_pixel_formats::ASTRA_PIXEL_FORMAT_DEPTH_MM);
	depthMode.set_fps(30);

	depthStream.set_mode(depthMode);

	auto newMode = depthStream.mode();
	printf("Depth mode: %dx%d @ %d -> %dx%d @ %d\n",
		   oldMode.width(), oldMode.height(), oldMode.fps(),
		   newMode.width(), newMode.height(), newMode.fps());

	return depthStream;
}

int main(int argc, char** argv)
{
	astra::initialize();

	const int w = 640;
	const int h = 480;

	int16_t minDepth = 0;
	int16_t maxDepth = 8000;

	bool switched = false;

	StreamSet ss;
	StreamReader r = ss.create_reader();
	DepthStream ds = configure_depth(r);
	ds.enable_registration(true);
	ds.start();

	sf::RenderWindow app(sf::VideoMode(w, h), "Astra min max depth bounds test");
	app.setKeyRepeatEnabled(false);

	int16_t* depthOrigin = new int16_t[w*h];

	while (app.isOpen()) {
		Frame f = r.get_latest_frame();
		const DepthFrame df = f.get<DepthFrame>();

		sf::Image img;
		img.create(w, h);

		for (int x = 0; x < w; x++) {
			for (int y = 0; y < h; y++) {
				const int index = x + y * w;
				if (switched) {
					const int16_t depth = depthOrigin[index] - df.data()[index];
					sf::Color c;
					if (depth >= minDepth && depth <= maxDepth) {
						c.r = 255;
						c.b = 255;
						c.g = 255;
					}
					else {
						c.r = 0;
						c.b = 0;
						c.g = 0;
					}
					img.setPixel(x, y, c);
				}
				else {
					const unsigned char color = df.data()[index] % 255;
					sf::Color c;
					c.r = color;
					c.b = color;
					c.g = color;
					img.setPixel(x, y, c);
				}
			}
		}

		sf::Texture tex;
		tex.create(w, h);
		tex.loadFromImage(img);
		sf::Sprite spr(tex);
	
		app.clear();
		app.draw(spr);
		app.display();

		sf::Event event;
		while (app.pollEvent(event)) {
			switch (event.key.code) {
				case sf::Keyboard::Escape: 
					app.close(); 
					break;
				case sf::Keyboard::S: 
					df.copy_to(depthOrigin);
					switched = true;
					cout << "depth origin: " << switched << endl; 
					break;
				case sf::Keyboard::Up:
					minDepth++;
					cout << "min:" << minDepth << endl;
					break;
				case sf::Keyboard::Down:
					if (minDepth > 0) minDepth--;
					else minDepth = 0;
					cout << "min:" << minDepth << endl;
					break;
				case sf::Keyboard::Right:
					maxDepth++;
					cout << "max:" << maxDepth << endl;
					break;
				case sf::Keyboard::Left:
					if (maxDepth > 0) maxDepth--;
					else maxDepth = 0;
					cout << "max:" << maxDepth << endl;
					break;
			}
		}
	}

	astra::terminate();
	delete[] depthOrigin;

	return 0;
}
