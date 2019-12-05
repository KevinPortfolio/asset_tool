#include "png.h"

//NOTE: Known static PNG names.
#define SIGNATURE 727905341920923785
#define IHDR 1229472850
#define IEND 1229278788
#define IDAT 1229209940

struct DeflateBlock
{
  uint32 Length;
  uint32 BytesPerPixel;
  uint32 EndIndexPos;
  uint32 ScanLinePos;
  uint32 ScanLineWidth;
  uint8 CurrentFilter;
};

static int32
PaethPredictor(int32 Previous, int32 Above, int32 AbovePrev)
{
  int32 p = Previous + Above - AbovePrev;
  int32 pa = Math_AbsVal(p - Previous);
  int32 pb = Math_AbsVal(p - Above);
  int32 pc = Math_AbsVal(p - AbovePrev);
  if ((pa <= pb) && (pa <= pc))
  {
    return Previous;
  }
  else if (pb <= pc)
  {
    return Above;
  }
  else
  {
    return AbovePrev;
  }
}

static void
NoFilter(uint32* Index, uint8* Data, uint32* ImgIndex,
	 uint8* ImageData, DeflateBlock* BlockState)
{
  ImageData[*ImgIndex] = Data[*Index];
  *ImgIndex = *ImgIndex + 1;
}

static void 
SubFilter(uint32* Index, uint8* Data, uint32* ImgIndex,
	  uint8* ImageData, DeflateBlock* BlockState) 
{
  if (BlockState->ScanLinePos > 2)
  {
    ImageData[*ImgIndex] = Data[*Index] +
      ImageData[*ImgIndex - BlockState->BytesPerPixel];
    *ImgIndex = *ImgIndex + 1;
  }
  else
  {
    ImageData[*ImgIndex] = Data[*Index];
    *ImgIndex = *ImgIndex + 1;
  }
}

static void 
UpFilter(uint32* Index, uint8* Data, uint32* ImgIndex,
	 uint8* ImageData, DeflateBlock* BlockState) 
{
  if (*Index > (BlockState->EndIndexPos -
		BlockState->Length + BlockState->ScanLineWidth))
  {
    ImageData[*ImgIndex] = Data[*Index] +
      ImageData[*ImgIndex - BlockState->ScanLineWidth];
    *ImgIndex = *ImgIndex + 1;
  }
  else
  {
    ImageData[*ImgIndex] = Data[*Index];
    *ImgIndex = *ImgIndex + 1;
  }
  
}

static void 
AverageFilter(uint32* Index, uint8* Data, uint32* ImgIndex,
	      uint8* ImageData, DeflateBlock* BlockState)
{
  if (*Index > (BlockState->EndIndexPos - 
		BlockState->Length + BlockState->ScanLineWidth))
  {
    if (BlockState->ScanLinePos > 2)
    {
      ImageData[*ImgIndex] = Data[*Index] +
	(int)Math_Floor((ImageData[*ImgIndex -
				   BlockState->BytesPerPixel] +
			 ImageData[*ImgIndex - BlockState->ScanLineWidth]) * 0.5f);
      *ImgIndex = *ImgIndex + 1;
    }
    else
    {
      ImageData[*ImgIndex] = Data[*Index] +
	(int)Math_Floor(ImageData[*ImgIndex -
				  BlockState->ScanLineWidth] * 0.5f);
      *ImgIndex = *ImgIndex + 1;
    }
  }
  else
  {
    if (BlockState->ScanLinePos > 2)
    {
      ImageData[*ImgIndex] = Data[*Index] +
	(int)Math_Floor((ImageData[*ImgIndex -
				   BlockState->BytesPerPixel] + 0) * 0.5f);
      *ImgIndex = *ImgIndex + 1;
    }
    else
    {
      ImageData[*ImgIndex] = Data[*Index];
      *ImgIndex = *ImgIndex + 1;
    }
  }
}

static void
PaethFilter(uint32* Index, uint8* Data, uint32* ImgIndex,
	    uint8* ImageData, DeflateBlock* BlockState)
{
  if (*Index > (BlockState->EndIndexPos -
		BlockState->Length + BlockState->ScanLineWidth))
  {
    if (BlockState->ScanLinePos > 2)
    {
      ImageData[*ImgIndex] = Data[*Index] +
	PaethPredictor(ImageData[*ImgIndex -
				 BlockState->BytesPerPixel],
		       ImageData[*ImgIndex - BlockState->ScanLineWidth],
		       ImageData[*ImgIndex - BlockState->ScanLineWidth -
				 BlockState->BytesPerPixel]);
      *ImgIndex = *ImgIndex + 1;
    }
    else
    {
      ImageData[*ImgIndex] = Data[*Index] + PaethPredictor(0,
							   ImageData[*ImgIndex - BlockState->ScanLineWidth], 0);
      *ImgIndex = *ImgIndex + 1;
    }
  }
  else
  {
    if (BlockState->ScanLinePos > 2)
    {
      ImageData[*ImgIndex] = Data[*Index] +
	PaethPredictor(ImageData[*ImgIndex -
				 BlockState->BytesPerPixel], 0, 0);
      *ImgIndex = *ImgIndex + 1;
    }
    else
    {
      ImageData[*ImgIndex] = Data[*Index] +
	PaethPredictor(0, 0, 0);
      *ImgIndex = *ImgIndex + 1;
    }
  }
}

void
PNG_Extract(byte* Data, byte** TextureData, uint32* Width, uint32* Height, uint8* BytesPerPixel)
{
  // TODO: Clean up Data if error.
  
  PNGProperties Properties;
  
  //NOTE: Confirm that the data is from a PNG file.
  unsigned long long* Signature = (unsigned long long*)Data;
  if (*Signature != SIGNATURE)
  {
    //TODO: Debug
  }
  
  //NOTE: Read the IHDR chunk.
  /*
    Width:              4 bytes
    Height:             4 bytes
    Bit depth:          1 byte
    Color type:         1 byte
    Compression method: 1 byte
    Filter method:      1 byte
    Interlace method:   1 byte
  */
  int32 ChunkLength = Data[11] | (Data[10] << 8) | (Data[9] << 16) |
    (Data[8] << 24);
  int32 ChunkName = Data[15] | (Data[14] << 8) | (Data[13] << 16) |
    (Data[12] << 24);
  
  if (ChunkName == IHDR)
  {
    *Width = (Data[19]) | (Data[18] << 8) | (Data[17] << 16) |
      (Data[16] << 24);
    *Height = (Data[23]) | (Data[22] << 8) | (Data[21] << 16) |
      (Data[20] << 24);
    Properties.BitDepth = Data[24];
    Properties.ColorType = Data[25];
    Properties.CompressionMethod = Data[26];
    Properties.FilterMethod = Data[27];
    Properties.InterlaceMethod = Data[28];
    *BytesPerPixel = Properties.BitDepth / 8;
    Properties.WidthInBytes = *Width *
      *BytesPerPixel;
  }
  else
  {
    //TODO: Debug
  }
  int32 ChunkCRC = Data[32] | (Data[31] << 8) | (Data[30] << 16) |
    (Data[29] << 24);
  
  if (*Width && *Height)
  {

    ImageData = 0;
    ImageData = Memory_Allocate(ImageData, Properties.WidthInBytes *
				Properties.Height);
    *TextureData = new unsigned char[Properties.WidthInBytes *
				     *Height];
  }
  else
  {
    //TODO: Debug, Technically not an error.
  }
  
  uint32 i = 33;
  uint32 ImgIndex = 0;
  
  //NOTE: Loop through all chunks
  bool EndExtraction = false;
  while (!EndExtraction)
  {
    ChunkLength = (Data[i + 3]) | (Data[i + 2] << 8) |
      (Data[i + 1] << 16) | (Data[i] << 24);
    ChunkName = Data[i + 7] | (Data[i + 6] << 8) |
      (Data[i + 5] << 16) | (Data[i + 4] << 24);
    
    i = i + 8;
    
    if (ChunkName == IEND)
    {
      EndExtraction = true;
    }
    else
    {
      if (ChunkName == IDAT)
      {
	//NOTE: ZLib Deflate/Inflate Compression information.
	//TODO: Handle more options from just 78 01 (like 78 9C or 78 DA).
	uint8 CompressionMethod = Data[i];
	i++;
	
	uint8 AdditionalFlags = Data[i];
	i++;
	
	//TODO: Handle the possibility of a DICTID?
	
	//NOTE: Index + ChunkLength - Two Flag Bytes - 4 ADLER32 Bytes
	uint32 ZlibDataLength = i + ChunkLength - 6;
	
	DeflateBlock BlockState;
	BlockState.BytesPerPixel = *BytesPerPixel;
	BlockState.ScanLinePos = Properties.WidthInBytes;// Forcing a line reset
	BlockState.ScanLineWidth = Properties.WidthInBytes;
	
	//NOTE: Loop through the image data (Deflate compress algorithm)
	while (i < ZlibDataLength)
	{
	  //NOTE: Block Header
	  /*
	    bit 1: Last block boolean.
	    bits 2-3:  00 - Raw data between 0 and 65,535 bytes in length.
	    01 - Static Huffman compressed block, pree-agreed tree.
	    10 - Compressed block with the Huffman table supplied.
	    11 - Reserved.
	  */
	  uint8 BlockHeader = Data[i];
	  i++;
	  
	  if ((BlockHeader << 7) == 0x00)
	  {
	    uint16 BlockLength = (Data[i]) |
	      (Data[i + 1] << 8);
	    i = i + 2;
	    
	    uint16 BlockLength1sComplement = (Data[i]) |
	      (Data[i + 1] << 8);
	    i = i + 2;
	    
	    BlockState.Length = BlockLength;
	    BlockState.EndIndexPos = BlockLength + i;
	    
	    while (i < BlockState.EndIndexPos)
	    {
	      if (BlockState.ScanLinePos < BlockState.ScanLineWidth)
	      {
		switch (BlockState.CurrentFilter)
		{
		case 0:
		  {
		    NoFilter(&i, Data, &ImgIndex, *TextureData,
			     &BlockState);
		  } break;
		case 1:
		  {
		    SubFilter(&i, Data, &ImgIndex,
			      *TextureData, &BlockState);
		  } break;
		case 2:
		  {
		    UpFilter(&i, Data, &ImgIndex,
			     *TextureData, &BlockState);
		  } break;
		case 3:
		  {
		    AverageFilter(&i, Data, &ImgIndex,
				  *TextureData, &BlockState);
		  } break;
		case 4:
		  {
		    PaethFilter(&i, Data, &ImgIndex,
				*TextureData, &BlockState);
		  } break;
		default:
		  {
		    //TODO: Error Handle
		  } break;
		}
		BlockState.ScanLinePos++;
		i++;
	      }
	      else // NOTE: New scanline
	      {
		BlockState.CurrentFilter = Data[i];
		BlockState.ScanLinePos = 0;
		i++;
	      }
	    }
	  }
	  else if ((BlockHeader << 7) == 0x80)
	  {
	    int16 BlockLength = (Data[i]) |
	      (Data[i + 1] << 8);
	    i = i + 2;
	    
	    int16 BlockLength1sComplement = (Data[i]) |
	      (Data[i + 1] << 8);
	    i = i + 2;
	  }
	}
	
	int32 ADLER32Checksum = (Data[i + 3]) | (Data[i + 2] << 8) |
	  (Data[i + 1] << 16) | (Data[i] << 24);
	i = i + 4;
      }
      else if (i == 900000)
      {
	break; //TODO: Debug
      }
      else
      {
	//NOTE: Increment the index past the chunk data.
	i = i + ChunkLength;
      }
      ChunkCRC = Data[i + 3] | (Data[i + 2] << 8) |
	(Data[i + 1] << 16) | (Data[i] << 24);;
      i = i + 4;
    }
  }
}