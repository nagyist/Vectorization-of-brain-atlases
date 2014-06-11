#include "bitmap.h"
#include "fstream"
#include "iostream"
#include "debug.h"
#include <string>
#include <algorithm>
#include <cmath>

//Constructor
Bitmap::Bitmap(std::string filename, pixel bgColor, bool bgColorProvided)
{
	uint h, w;
	std::vector<uchar> v;	//the raww pixel

	decodeOneStep(filename.c_str(), h, w, v);
	//the pixels are now in the vector "v", 4 bytes per pixel, ordered RGBARGBA

	//Initialization and allocation of private variables
	intialize(&orig, h, w);
	intialize(&pre, h + 2, w + 2);
	intialize(&pop, 2 * (h + 2), 2 * (w + 2));
	pixToNodeMap = new int*[2 * (h + 2)];
	for(uint i = 0; i < 2 * (h + 2); ++i)
	{
		pixToNodeMap[i] = new int[2 * (w + 2)];
	}
	numControlPoints = 0;

	#ifdef _TEST_1_ //checks if file read crrectly
	//file to store r g b values of each pixel in row major order under test 1
	std::string path = ROOT_DIR;
	std::ofstream ofsTest1((path + "/check/test_1_cpp.txt").c_str(), std::ofstream::out);
	ofsTest1 << h << std::endl;
	ofsTest1 << w << std::endl;
	#endif

	//Assigning values to the pixels of original image matrix
	for(uint i = 0; i < h; ++i)
	{
		for(uint j = 0; j < w; ++j)
		{
			orig->pixMap[i][j].r = v[4 * (i * w + j)];
			orig->pixMap[i][j].g = v[4 * (i * w + j) + 1];
			orig->pixMap[i][j].b = v[4 * (i * w + j) + 2];

			#ifdef _TEST_1_
			//print r g b int values in new lines
			ofsTest1 << (uint)v[4 * (i * w + j)] << std::endl; 
			ofsTest1 << (uint)v[4 * (i * w + j) + 1] << std::endl;
			ofsTest1 << (uint)v[4 * (i * w + j) + 2] << std::endl;
			#endif
		}
	}

	#ifdef _TEST_1_
	//close test_1_cpp.txt file
	ofsTest1.close();
	#endif	

	//borderPixel initialization
	if(bgColorProvided)
	{
		borderPixel.r = bgColor.r;
		borderPixel.g = bgColor.g;
		borderPixel.b = bgColor.b;
	}
	else
	{
		//median of 4 corners of original image
		uint a[4];
		a[0] = orig->pixMap[0][0].r + orig->pixMap[0][0].g * 256 + orig->pixMap[0][0].b * 256 * 256;
		a[1] = orig->pixMap[0][w-1].r + orig->pixMap[0][w-1].g * 256 + orig->pixMap[0][w-1].b * 256 * 256;
		a[2] = orig->pixMap[h-1][0].r + orig->pixMap[h-1][0].g * 256 + orig->pixMap[h-1][0].b * 256 * 256;
		a[3]= orig->pixMap[h-1][w-1].r + orig->pixMap[h-1][w-1].g * 256 + orig->pixMap[h-1][w-1].b * 256 * 256;

		std::sort(a, a+4);
		double med = (a[1] + a[2])*0.5;
		uint median = floor(med);
		borderPixel.b = median/(256*256);
		median = median - borderPixel.b * 256 * 256;
		borderPixel.g = median/256;
		median = median - borderPixel.g * 256;
		borderPixel.r = median;
	}
}

/*
 * processes the orig image
 */
void Bitmap::processImage(uint tolerance)
{
	preprocess();
	popoutBoundaries();

	#ifdef _EVAL_1_
	//generate some image
	std::string path = ROOT_DIR;
	std::vector<unsigned char> image;
	image.resize(pop->width * pop->height * 4);
	for(uint y = 0; y < pop->height; y++)
	{
		for(uint x = 0; x < pop->width; x++)
		{
			image[4 * pop->width * y + 4 * x + 0] = pop->pixMap[y][x].r;
			image[4 * pop->width * y + 4 * x + 1] = pop->pixMap[y][x].g;
			image[4 * pop->width * y + 4 * x + 2] = pop->pixMap[y][x].b;
			image[4 * pop->width * y + 4 * x + 3] = 255;
		}
	}

	encodeOneStep((path + "/check/eval_1_popout.png").c_str(), image, pop->width, pop->height);
	image.clear();
	#endif

	codedImage = new CodeImage(pop);
	graph = new Graph(pop->height, pop->width, *codedImage->getColCode(), pop);
	detectControlPoints();
	formAdjacencyList();
	graph->formLineSegments();
	graph->assignCurveNumToRegion();
	graph->preprocessLineSegments();
	graph->formCurves(tolerance);
	graph->processRegions();
}

//writes svg output in outFileName by moving control to writeOutput method of graph
void Bitmap::writeOuputSVG(std::string outFileName)
{
	graph->writeOuput(outFileName, borderPixel);
}

//Destructor
Bitmap::~Bitmap()
{
	//delete pixToNodeMap
	for(uint i = 0; i < pop->height; ++i)
	{
		delete[] pixToNodeMap[i];
	}
	delete[] pixToNodeMap;

	//delete orig
	del(orig);

	//delete pre
	del(pre);

	//delete pop
	del(pop);

	//delete coded image
	delete[] codedImage;

	//delete graph
	delete[] graph;
}

//deletes an image matrix
void Bitmap::del(ImageMatrix *a)
{
	for(uint i = 0; i < a->height; ++i)
	{
		delete[] a->pixMap[i];
	}
	delete[] a->pixMap;
	delete a;
}

//Decode from disk to raw pixels with a single function call
void Bitmap::decodeOneStep(const char* filename, uint &height, uint &width, std::vector<uchar> &image)
{
  //decode
  unsigned error = lodepng::decode(image, width, height, filename);

  //if there's an error, display it
  if(error) 
  	std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
}

//Initialize image matrix
void Bitmap::intialize(ImageMatrix **m, uint h, uint w)
{
	*m = new ImageMatrix;
	(*m)->height = h;
	(*m)->width = w;
	(*m)->pixMap = new pixel*[h];
	for(uint i = 0; i < h; ++i)
	{
		(*m)->pixMap[i] = new pixel[w];
	}
}

/*
 * Appends row at top and bottom and col at left and right of 
 * orig image with black (0,0,0) pixels
 */
void Bitmap::preprocess()
{
	uint h = pre->height;
	uint w = pre->width;
	int M[3][256] = {{0}};

	for(uint i = 0; i < h; ++i)
	{
		for(uint j = 0; j < w; ++j)
		{
			if(i == 0 || j == 0 || i == h - 1 || j == w - 1)
			{
				pre->pixMap[i][j].r = borderPixel.r;
				pre->pixMap[i][j].g = borderPixel.g;
				pre->pixMap[i][j].b = borderPixel.b;
			}
			else
			{
				pre->pixMap[i][j].r = orig->pixMap[i - 1][j - 1].r;
				pre->pixMap[i][j].g = orig->pixMap[i - 1][j - 1].g;
				pre->pixMap[i][j].b = orig->pixMap[i - 1][j - 1].b;
				M[0][orig->pixMap[i - 1][j - 1].r] = -1;
				M[0][orig->pixMap[i - 1][j - 1].g] = -1;
				M[0][orig->pixMap[i - 1][j - 1].b] = -1;				
			}
		}
	}

	//Getting unique borderPixel and boundaryPixel
	bool outOfLoop = false;
	for(uint i = 0; i < 3; ++i)
	{
		for(uint j = 0; j < 256; ++j)
		{
			if(M[i][j] != -1)
			{
				switch(i)
				{
					case 0:
						boundaryPixel.r = j;
						boundaryPixel.g = boundaryPixel.b = 255;
						break;
					case 1:
						boundaryPixel.g = j;
						boundaryPixel.r = boundaryPixel.b = 255;
						break;
					case 2:
						boundaryPixel.b = j;
						boundaryPixel.g = boundaryPixel.r = 255;
						break;
				}
				outOfLoop = true;
				break;
			}
		}
		if(outOfLoop)
			break;
	}
}

/*
 * Please refer doc for the algorithm for popping out boundaries.
 */
void Bitmap::popoutBoundaries()
{
	uint h = pop->height;
	uint w = pop->width;
	
	for(uint i = 0; i < h; ++i)
	{
		for(uint j = 0; j < w; ++j)
		{
			pop->pixMap[i][j].r = borderPixel.r;
			pop->pixMap[i][j].g = borderPixel.g;
			pop->pixMap[i][j].b = borderPixel.b;
		}
	}

	for(uint i = 1; i < h * 0.5; ++i)
	{
		uint focus_i = 2 * i + 1;
		for(uint j = 1; j < w * 0.5; ++j) 
		{
			uint focus_j = 2 * j + 1;

			pop->pixMap[focus_i][focus_j].r = pre->pixMap[i][j].r;
			pop->pixMap[focus_i][focus_j].g = pre->pixMap[i][j].g;
			pop->pixMap[focus_i][focus_j].b = pre->pixMap[i][j].b;

			if(!ifEqualPixel(pre->pixMap[i][j], pre->pixMap[i][j - 1]))
			{
				pop->pixMap[focus_i][focus_j - 1].r = boundaryPixel.r;
				pop->pixMap[focus_i][focus_j - 1].g = boundaryPixel.g;
				pop->pixMap[focus_i][focus_j - 1].b = boundaryPixel.b;
			}
			else
			{
				pop->pixMap[focus_i][focus_j - 1].r = pre->pixMap[i][j - 1].r;
				pop->pixMap[focus_i][focus_j - 1].g = pre->pixMap[i][j - 1].g;
				pop->pixMap[focus_i][focus_j - 1].b = pre->pixMap[i][j - 1].b;
			}
			if(!ifEqualPixel(pre->pixMap[i][j], pre->pixMap[i - 1][j - 1]))
			{
				pop->pixMap[focus_i - 1][focus_j - 1].r = boundaryPixel.r;
				pop->pixMap[focus_i - 1][focus_j - 1].g = boundaryPixel.g;
				pop->pixMap[focus_i - 1][focus_j - 1].b = boundaryPixel.b;
			}
			else
			{
				pop->pixMap[focus_i - 1][focus_j - 1].r = pre->pixMap[i - 1][j - 1].r;
				pop->pixMap[focus_i - 1][focus_j - 1].g = pre->pixMap[i - 1][j - 1].g;
				pop->pixMap[focus_i - 1][focus_j - 1].b = pre->pixMap[i - 1][j - 1].b;
			}
			if(!ifEqualPixel(pre->pixMap[i][j], pre->pixMap[i - 1][j]))
			{
				pop->pixMap[focus_i - 1][focus_j].r = boundaryPixel.r;
				pop->pixMap[focus_i - 1][focus_j].g = boundaryPixel.g;
				pop->pixMap[focus_i - 1][focus_j].b = boundaryPixel.b;
			}
			else
			{
				pop->pixMap[focus_i - 1][focus_j].r = pre->pixMap[i - 1][j].r;
				pop->pixMap[focus_i - 1][focus_j].g = pre->pixMap[i - 1][j].g;
				pop->pixMap[focus_i - 1][focus_j].b = pre->pixMap[i - 1][j].b;
			}
		}
	}

	#ifdef _TEST_2_
	//file to store r g b values of each pixel in row major order under test 2
	std::string path = ROOT_DIR;
	std::ofstream ofsTest2((path + "/check/test_2_cpp.txt").c_str(), std::ofstream::out);
	ofsTest2 << h << std::endl;
	ofsTest2 << w << std::endl;

	for(uint i = 0; i < h; ++i)
	{
		for(uint j = 0; j < w; ++j)
		{
			//print r g b int values in new lines
			ofsTest2 << (int)pop->pixMap[i][j].r << std::endl;
			ofsTest2 << (int)pop->pixMap[i][j].g << std::endl;
			ofsTest2 << (int)pop->pixMap[i][j].b << std::endl;
		}
	}

	//close test_2_cpp.txt file
	ofsTest2.close();
	#endif
}

/*
 * Detects control points which are boundary(white) points where 3 or more regions meet
 * Initializes the graph with its vertices
*/
void Bitmap::detectControlPoints()
{
	uint h = pop->height;
	uint w = pop->width;
	uint vertexCounter = 0;
	std::vector<pixel> temp;

	#ifdef _EVAL_2_
	std::vector<unsigned char> image;
	image.resize(w * h * 4);
	#endif

	#ifdef _TEST_4_
	//file to store coordinate values of control points under test 4
	std::string path = ROOT_DIR;
	std::ofstream ofsTest4((path + "/check/test_4_cpp.txt").c_str(), std::ofstream::out);
	ofsTest4 << h << std::endl;
	ofsTest4 << w << std::endl;
	#endif

	for(uint i = 1; i < h - 1; ++i)
	{
		for(uint j = 1; j < w - 1; ++j)
		{
			#ifdef _EVAL_2_
			image[4 * w * i + 4 * j + 0] = pop->pixMap[i][j].r;
			image[4 * w * i + 4 * j + 1] = pop->pixMap[i][j].g;
			image[4 * w * i + 4 * j + 2] = pop->pixMap[i][j].b;
			image[4 * w * i + 4 * j + 3] = 255;
			#endif

			//if pixel at (i,j) is a white pixel
			if(ifEqualPixel(pop->pixMap[i][j], boundaryPixel))
			{
				pixToNodeMap[i][j] = vertexCounter;
				//adding a vertex in graph
				graph->addVertex(vertexCounter, i, j, 0, 0, 0);
				temp.clear();
				//if pixel at (i,j-1) is a unique (newly found) adjacent region pixel
				if(checkUniqueRegionPixel(pop->pixMap[i][j - 1], temp))
					//adds adjacent region code to adjacent region vector of vertex in graph
					graph->addRegion(vertexCounter, codedImage->getMatrix()[i][j - 1]);	

				if(checkUniqueRegionPixel(pop->pixMap[i][j + 1], temp))
					graph->addRegion(vertexCounter, codedImage->getMatrix()[i][j + 1]);

				if(checkUniqueRegionPixel(pop->pixMap[i - 1][j - 1], temp))
					graph->addRegion(vertexCounter, codedImage->getMatrix()[i - 1][j - 1]);

				if(checkUniqueRegionPixel(pop->pixMap[i - 1][j + 1], temp))
					graph->addRegion(vertexCounter, codedImage->getMatrix()[i - 1][j + 1]);

				if(checkUniqueRegionPixel(pop->pixMap[i + 1][j - 1], temp))
					graph->addRegion(vertexCounter, codedImage->getMatrix()[i + 1][j - 1]);

				if(checkUniqueRegionPixel(pop->pixMap[i + 1][j + 1], temp))
					graph->addRegion(vertexCounter, codedImage->getMatrix()[i + 1][j + 1]);

				if(checkUniqueRegionPixel(pop->pixMap[i - 1][j], temp))
					graph->addRegion(vertexCounter, codedImage->getMatrix()[i - 1][j]);

				if(checkUniqueRegionPixel(pop->pixMap[i + 1][j], temp))
					graph->addRegion(vertexCounter, codedImage->getMatrix()[i + 1][j]);


				if(temp.size() >= 3)
				{
					//set node with index = vertex counter = a control point in graph
					graph->setControlPoint(vertexCounter, 1);
					#ifdef _TEST_4_
					//print coordinates of control point
					ofsTest4 << i << std::endl;
					ofsTest4 << j << std::endl;
					#endif
					#ifdef _EVAL_2_
					image[4 * w * i + 4 * j + 0] = 0;
					image[4 * w * i + 4 * j + 1] = 0;
					image[4 * w * i + 4 * j + 2] = 255;
					image[4 * w * i + 4 * j + 3] = 255;
					#endif
				}

				vertexCounter++;
			}
			else
			{
				//not a white pixel
				pixToNodeMap[i][j] = -1;
			}
		}
	}

	#ifdef _TEST_4_
	//close file
	ofsTest4.close();
	#endif

	#ifdef _EVAL_2_
	encodeOneStep((path + "/check/eval_2_controlPt.png").c_str(), image, w, h);
	image.clear();
	#endif

	temp.clear();
}

/*
 * Returns 1 if pixel "a" is not a white pixel and is not contained in vector v.
 * vector v contains the region pixels adjacent to a node = white vertex detected yet.
 * if "a" is also a unique region pixel adjacent to the node then returns 1 else 0. 
 */
int Bitmap::checkUniqueRegionPixel(pixel a, std::vector<pixel> &v)
{
	int ret = 0;
	if(!(a.r == boundaryPixel.r && a.g == boundaryPixel.g && a.b == boundaryPixel.b))
	{
		uint flag = 0;
		for(uint i = 0; i < v.size(); ++i)
		{
			if(ifEqualPixel(a, v[i]))
			{
				flag = 1;
				break;
			}
		}
		if(flag == 0)
		{
			v.push_back(a);
			ret = 1;
		}
	}
	
	return ret;
}

/*
 * forms adjacency list in graph.
 * connects two vertices if they are adjacent.
 * the connection is bidirectional(undirected garph).
 * Since only white pixels are nodes/vertices, we loop over all the pixels,
 * check if the pixel is white,
 * if yes: then connects this pixel/vertex with any adjacent white pixel/vertex
 */
void Bitmap::formAdjacencyList()
{
	uint h = pop->height;
	uint w = pop->width;

	for(uint i = 1; i < h - 1; ++i)
	{
		for(uint j = 1; j < w - 1; ++j)
		{
			if(ifEqualPixel(pop->pixMap[i][j], boundaryPixel))
			{
				if(ifEqualPixel(pop->pixMap[i - 1][j], boundaryPixel))
					graph->addEdge(pixToNodeMap[i][j], pixToNodeMap[i - 1][j]);

				if(ifEqualPixel(pop->pixMap[i + 1][j], boundaryPixel))
					graph->addEdge(pixToNodeMap[i][j], pixToNodeMap[i + 1][j]);

				if(ifEqualPixel(pop->pixMap[i - 1][j - 1], boundaryPixel))
					graph->addEdge(pixToNodeMap[i][j], pixToNodeMap[i - 1][j - 1]);

				if(ifEqualPixel(pop->pixMap[i - 1][j + 1], boundaryPixel))
					graph->addEdge(pixToNodeMap[i][j], pixToNodeMap[i - 1][j + 1]);

				if(ifEqualPixel(pop->pixMap[i + 1][j - 1], boundaryPixel))
					graph->addEdge(pixToNodeMap[i][j], pixToNodeMap[i + 1][j - 1]);

				if(ifEqualPixel(pop->pixMap[i + 1][j + 1], boundaryPixel))
					graph->addEdge(pixToNodeMap[i][j], pixToNodeMap[i + 1][j + 1]);

				if(ifEqualPixel(pop->pixMap[i][j - 1], boundaryPixel))
					graph->addEdge(pixToNodeMap[i][j], pixToNodeMap[i][j - 1]);

				if(ifEqualPixel(pop->pixMap[i][j + 1], boundaryPixel))
					graph->addEdge(pixToNodeMap[i][j], pixToNodeMap[i][j + 1]);
			}
		}
	}
}
