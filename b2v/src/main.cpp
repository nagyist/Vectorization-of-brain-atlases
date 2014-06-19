/*
 * Google Summer Of Code
 * Project: Real-time vectorization of brain-atlases
 * Mentoring Organization: International Neuroinformatics Coordinating Facility
 * Mentors: Rembrandt Bakker, Piotr Majka
 * Student: Dhruv Kohli
 * blog: https://rtvba.weebly.com
 */

#include <iostream>
#include <string>
#include <cstdlib>
#include "bitmap.h"
#include "util.h"

int main(int argc, char **argv)
{
	std::string filename = "";
	std::string outFileName = "";
	double toleranceCurve = 2;
	double toleranceLine = 0;
	std::string color;
	pixel bgColor;
	bool bgColorProvided = false;

	//parsing arguments
	int i = argc;
	while(i > 1)
	{
		std::string temp(argv[i - 1]);
		if(temp == "-i" || temp == "--input-file")
		{
			if(argc != i)
				filename = argv[i];
			else
			{
				std::cout << "invalid input file" << std::endl;
				exit(1);
			}
		}
		else if(temp == "-o" || temp == "--output-file")
		{
			if(argc != i)
				outFileName = argv[i];
			else
			{
				std::cout << "invalid output file" << std::endl;
				exit(1);
			}
		}
		else if(temp == "-t" || temp == "--curve-tolerance")
		{
			if(argc != i)
				toleranceCurve = atoi(argv[i]);
			else
			{
				std::cout << "invalid tolerance for Curve" << std::endl;
				exit(1);
			}
		}
		else if(temp == "-s" || temp == "--line-tolerance")
		{
			if(argc != i)
				toleranceLine = atoi(argv[i]);
			else
			{
				std::cout << "invalid tolerance for Line" << std::endl;
				exit(1);
			}
		}
		else if(temp == "-c" || temp == "--bg-color")
		{
			if(argc != i)
			{
				color = argv[i];
				if(color.substr(0, 1) == "#")
				{
					bgColor = hexToRGB(color);
				}
				else if(color.substr(0, 3) == "rgb")
				{
					bgColor = parseRGB(color);
				}
				else
				{
					std::cout << "invalid background color: Either #RRGGBB or rgb(R,G,B) must be provided" << std::endl;
					exit(1);
				}
				bgColorProvided = true;
			}
			else
			{
				std::cout << "invalid background color: Either #RRGGBB or rgb(R,G,B) must be provided" << std::endl;
				exit(1);
			}
		}
		else if(temp == "-h" || temp == "--help")
		{
			std::cout << "b2v: Transforms bitmaps to vector graphics\n\nUsage:b2v [options] [filename...]\nDescription\n\nArguments: \n-h, --help \n\t\t show this help message and exit\n-i, --input-file <filename> \n\t\t read input PNG Bitmap image from this file\n-o, --output-file <filename> \n\t\t write output to this file(default=ouput.svg)\n-t, --curve-tolerance <n>  \n\t\t Bezier Curve Fitting tolerance(default=2)\n-s, --line-tolerance <n> \n\t\t Line Fitting tolerance(default=0)\n-c, --bg-color <\"#RRGGBB\"> OR <\"rgb(R,G,B)\"> \n\t\t Background Color in base 16 or 10(default=based on median of colors of four corner points of input image)" << std::endl;
			exit(1);
		}
		i--;
	}

	if(toleranceCurve < 0)
	{
		std::cout << "invalid tolerance for curve: must be greater than zero" << std::endl;
		exit(1);
	}

	if(toleranceLine < 0)
	{
		std::cout << "invalid tolerance for line: must be greater than zero" << std::endl;
		exit(1);
	}

	if(outFileName == "")
	{
		outFileName = "ouput.svg";
	}

	if(filename == "")
	{
		std::cout << "input file not provided: use -i filename.png" << std::endl;
		exit(1);
	}

	Bitmap *inputBitmap = new Bitmap(filename, bgColor, bgColorProvided);
	inputBitmap->processImage(toleranceCurve, toleranceLine);
	inputBitmap->writeOuputSVG(outFileName);
	
	return 0;
}

