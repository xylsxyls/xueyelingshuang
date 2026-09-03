#include "PdfEngineBitmap.h"

PdfEngineBitmap::PdfEngineBitmap() :
width(0),
height(0),
stride(0)
{

}

void PdfEngineBitmap::clear()
{
    width = 0;
    height = 0;
    stride = 0;
    pixels.clear();
}