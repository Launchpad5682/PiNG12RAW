/*
    PiNG12RAW converts 12bit RGGB RAW image into 8 bit PNGs
    Initial Author: Supragya Raj (supragyaraj@gmail.com)
*/

#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>
#include "LodePNG/lodepng.h"

// Dictates how many LSBs are lost while conversion from 12bit input to the final outputs
// Generally for 12 bit input has to be converted to 8 bit output. right shift = 4 while left shift = 0
// NOTE: code not extended for 16bit outputs

const int L_SHIFTING_FACTOR = 0;    //Not used uptil now
const int R_SHIFTING_FACTOR = 4;

// Input file location
const char INPUT_FILE[] = "test_image/test.raw12";

// Width and Height of the image to be processes
const int WIDTH = 4096;
const int HEIGHT = 3072;

// Vector to PNG exporter (LodePNG)
// Parameters:
//      filename:   Exported PNG file filename
//      image:      an RGBA value image Vector
//      width:      width of the image to save
//      height:     height of the image to save
// Returns:
//      NA
void savePNGtoDisk(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height);

// Extracts a debayered image from RGGB channel (grayscale)
// Parameters:
//      r:      grayscale RGBA image vector for R channel
//      g1:     grayscale RGBA image vector for G1 channel
//      g2:     grayscale RGBA image vector for G2 channel
//      b:      grayscale RGBA image vector for B channel
//      c_width:  width of the channel
//      c_height: height of the channel
// Returns:
//      vector<unsigned char> result: result image(debayered)
std::vector<unsigned char> debayer_using_channels(std::vector<unsigned char> r,
                                                std::vector<unsigned char> g1,
                                                std::vector<unsigned char> g2,
                                                std::vector<unsigned char> b,
                                                unsigned int width, unsigned int height);


int main(){

    // A friendly welcome screen, to show that things have started working
    std::cout<<"PiNG12RAW Working... "<<std::endl;

    // Open the input file and make it ready
    std::ifstream pFile (INPUT_FILE, std::ios::binary|std::ios::in);
    if (!pFile){
      std::cout<<"Cannot open raw image file, exiting."<<std::endl;
      exit(-1);
    }

    std::vector<unsigned char> imageredgr, imagegreen1gr, imagegreen2gr, imagebluegr;
    imageredgr.resize(WIDTH * HEIGHT);
    imagegreen1gr.resize(WIDTH * HEIGHT);
    imagegreen2gr.resize(WIDTH * HEIGHT);
    imagebluegr.resize(WIDTH * HEIGHT);

    uint8_t eightbits[3];
    uint16_t left,right;

    uint8_t left8bit, right8bit;

    for(unsigned int row = 0; row < HEIGHT; row++){
      for(unsigned int col = 0; col < WIDTH/2; col++ ){

        //Read 3 eightbit values
        for(int bt = 0; bt < 3; bt++)
          pFile.read((char*)&eightbits[bt], sizeof(uint8_t));


        //Retrieve left and right side
        left = right = 0;
        left = ((eightbits[0]<<4)|(eightbits[1]&0x40)>>4);
        right = ((eightbits[1]&0x0F)<<8|(eightbits[2]));

        //Find the bit values
        left8bit = right8bit = 0;
        //Left8bits are : ((eightbits[0]<<4)|(eightbits[1]&0x40)>>4)>>4 so, effectively eightbits[0]
        left8bit = left>>R_SHIFTING_FACTOR;
        //Right8bits are: ((eightbits[1]&0x0F)<<4|(eightbits[2]))>>$ so effectively ((eightbits[1]&0x0F)|(eightbits[2])>>4)
        right8bit = right>>R_SHIFTING_FACTOR;

        //Set the values
        //on the even lines every second sample is a 'red' and on the odd lines every second a 'blue'

        if(row%2 == 0){
          for(int k=0; k<3; k++){
            imageredgr[4*((row/2)*WIDTH/2 + col) + k] = (int)left8bit;
            imagegreen1gr[4*((row/2)*WIDTH/2 + col) + k] = (int)right8bit;
          }
          imageredgr[4*((row/2)*WIDTH/2 + col) + 3] = 255;
          imagegreen1gr[4*((row/2)*WIDTH/2 + col) + 3] = 255;
        }
        else{
          for(int k=0;k<3;k++){
            imagegreen2gr[4*((row/2)*WIDTH/2 + col) + k] = (int)left8bit;
            imagebluegr[4*((row/2)*WIDTH/2 + col) + k] = (int)right8bit;
          }
          imagegreen2gr[4*((row/2)*WIDTH/2 + col) + 3] = 255;
          imagebluegr[4*((row/2)*WIDTH/2 + col) + 3] = 255;
        }



    }
}
savePNGtoDisk("results/r_grayscale.png", imageredgr, WIDTH/2, HEIGHT/2);
savePNGtoDisk("results/g1_grayscale.png", imagegreen1gr, WIDTH/2, HEIGHT/2);
savePNGtoDisk("results/g2_grayscale.png", imagegreen2gr, WIDTH/2, HEIGHT/2);
savePNGtoDisk("results/b_grayscale.png", imagebluegr, WIDTH/2, HEIGHT/2);
std::vector<unsigned char> debayeredimg = debayer_using_channels(imageredgr,imagegreen1gr,imagegreen2gr,imagebluegr, WIDTH/2, HEIGHT/2);
savePNGtoDisk("results/debayeredimg.png", debayeredimg, WIDTH, HEIGHT);
std::cout<<"done. Check [results] folder for new imgs."<<std::endl;

pFile.close();

return 0;
}

void savePNGtoDisk(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height)
{
    unsigned int error = lodepng::encode(filename, image, width, height);
    if(error)
        std::cout << "LodePNG encoder: Error occured while generating ["<<filename<<"] "<< error << ": "<< lodepng_error_text(error) << std::endl;
    else
        std::cout<<"LodePNG encoder: Success in generating ["<<filename<<"]"<<std::endl;
}
std::vector<unsigned char> debayer_using_channels(std::vector<unsigned char> r,
                                                std::vector<unsigned char> g1,
                                                std::vector<unsigned char> g2,
                                                std::vector<unsigned char> b,
                                                unsigned int c_width, unsigned int c_height){

    // Demarcation: op_width and op_height represent the output image (RGB) height and widht while c_width and c_height represent channel width and height
    unsigned int op_height = c_height*2;
    unsigned int op_width = c_width*2;

    std::vector<unsigned char> result;
    result.resize(4*(op_width)*(op_height));

    // Simple interpolation
    for(unsigned int i=0; i<op_height; i++){
        for(unsigned int j=0; j<op_width; j++){
            result[4*(i*op_width+j) + 0] = r[4*((i/2)*c_width+(j/2)) + 0];
            result[4*(i*op_width+j) + 1] = (g1[4*((i/2)*c_width+(j/2)) + 0] + g2[4*((i/2)*c_width+(j/2)) + 0])/2;
            result[4*(i*op_width+j) + 2] = b[4*((i/2)*c_width+(j/2)) + 0];
            result[4*(i*op_width+j) + 3] = 255;
        }
    }
    return result;
}
