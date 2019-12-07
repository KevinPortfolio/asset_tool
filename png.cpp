
//NOTE: Known static PNG names.
#define SIGNATURE 727905341920923785
#define IHDR 1229472850
#define IEND 1229278788
#define IDAT 1229209940

struct PNGProperties
{
  uint32 width;
  uint32 height;
  uint32 width_in_bytes;
  uint32 bytes_per_pixel;
  unsigned char bit_depth;
  unsigned char color_type;
  unsigned char compression_method;
  unsigned char filter_method;
  unsigned char interlace_method;
};

struct DeflateBlock
{
  uint32 length;
  uint32 bytes_per_pixel;
  uint32 end_index_pos;
  uint32 scan_line_pos;
  uint32 scan_line_width;
  uint8 current_filter;
};

static inline int32
png_math_abs_val(int32 value) {  return((value < 0) ? -value: value); }

static inline int32
png_math_floor(float value){ return (int32)value; }

static int32
paeth_predictor(int32 previous, int32 above, int32 above_prev)
{
  int32 p = previous + above - above_prev;
  int32 pa = png_math_abs_val(p - previous);
  int32 pb = png_math_abs_val(p - above);
  int32 pc = png_math_abs_val(p - above_prev);

  if ((pa <= pb) && (pa <= pc))
    return previous;
  else if (pb <= pc)
    return above;
  else
    return above_prev;
}

static inline void
no_filter(uint32* index, uint8* data, uint32* image_index, uint8* image_data, DeflateBlock* block_state)
{
  image_data[*image_index] = data[*index];
  *image_index = *image_index + 1;
}

static void 
sub_filter(uint32* index, uint8* data, uint32* image_index, uint8* image_data, DeflateBlock* block_state) 
{
  if (block_state->scan_line_pos > 2)
  {
    image_data[*image_index] = data[*index] + image_data[*image_index - block_state->bytes_per_pixel];
    *image_index = *image_index + 1;
    
  }
  else
  {
    image_data[*image_index] = data[*index];
    *image_index = *image_index + 1;
  }
}

static void 
up_filter(uint32* index, uint8* data, uint32* image_index, uint8* image_data,
	 DeflateBlock* block_state) 
{
  if (*index > (block_state->end_index_pos - block_state->length + block_state->scan_line_width))
  {
    image_data[*image_index] = data[*index] + image_data[*image_index - block_state->scan_line_width];
    *image_index = *image_index + 1;
    
  }
  else
  {
    image_data[*image_index] = data[*index];
    *image_index = *image_index + 1;
  }
  
}

static void 
average_filter(uint32* index, uint8* data, uint32* image_index, uint8* image_data, DeflateBlock* block_state)
{
  
  if (*index > (block_state->end_index_pos - block_state->length + block_state->scan_line_width))
  {
    if (block_state->scan_line_pos > 2)
    {
      image_data[*image_index] = data[*index] +
        png_math_floor((image_data[*image_index - block_state->bytes_per_pixel] +
			 image_data[*image_index - block_state->scan_line_width]) * 0.5f);
      *image_index = *image_index + 1;
      
    }
    else
    {
      image_data[*image_index] = data[*index] +
	png_math_floor(image_data[*image_index - block_state->scan_line_width] * 0.5f);
      *image_index = *image_index + 1;
    }
  }
  else
  {
    if (block_state->scan_line_pos > 2)
    {
      image_data[*image_index] = data[*index] + 0.5f *
        png_math_floor((image_data[*image_index - block_state->bytes_per_pixel] + 0));
      *image_index = *image_index + 1;
      
    }
    else
    {
      image_data[*image_index] = data[*index];
      *image_index = *image_index + 1;
    }
  }
}

static void
paeth_filter(uint32* index, uint8* data, uint32* image_index, uint8* image_data, DeflateBlock* block_state)
{
  if (*index > (block_state->end_index_pos - block_state->length + block_state->scan_line_width))
  {
    if (block_state->scan_line_pos > 2)
    {
      image_data[*image_index] = data[*index] +
	paeth_predictor(image_data[*image_index - block_state->bytes_per_pixel],
		        image_data[*image_index - block_state->scan_line_width],
		        image_data[*image_index - block_state->scan_line_width -
				    block_state->bytes_per_pixel]);
      *image_index = *image_index + 1;
      
    }
    else
    {
      image_data[*image_index] = data[*index] +
	paeth_predictor(0, image_data[*image_index - block_state->scan_line_width], 0);
      *image_index = *image_index + 1;
    }
  }
  else
  {
    
    if (block_state->scan_line_pos > 2)
    {
      image_data[*image_index] = data[*index] +
	paeth_predictor(image_data[*image_index - block_state->bytes_per_pixel], 0, 0);
      *image_index = *image_index + 1;
    }
    else
    {
      image_data[*image_index] = data[*index] + paeth_predictor(0, 0, 0);
      *image_index = *image_index + 1;
    }
  }
}

void
png_extract(byte* data, byte** texture_data, uint32* width, uint32* height, uint8* bytes_per_pixel)
{
  if (!data || !texture_data || !width || !height || !bytes_per_pixel)
  {
    // ERROR: Invalid inputs
    // TODO: Change function parameters to avoid this check.
  }
  PNGProperties properties;
  
  unsigned long long* signature = (unsigned long long*)data; 
  if (*signature != SIGNATURE)
  {
    // ERROR: not a png file or an incomplete png header.
  }

  int32 chunk_length = data[11] | (data[10] << 8) | (data[9] << 16) | (data[8] << 24);
  int32 chunk_name = data[15] | (data[14] << 8) | (data[13] << 16) | (data[12] << 24);
  
  if (chunk_name == IHDR)
  {    
  /* IHDR header chunk
    width:              4 bytes
    height:             4 bytes
    Bit depth:          1 byte
    Color type:         1 byte
    Compression method: 1 byte
    Filter method:      1 byte
    Interlace method:   1 byte
  */    
    *width = (data[19]) | (data[18] << 8) | (data[17] << 16) | (data[16] << 24);
    *height = (data[23]) | (data[22] << 8) | (data[21] << 16) | (data[20] << 24);
    
    properties.bit_depth = data[24];
    properties.color_type = data[25];
    properties.compression_method = data[26];
    properties.filter_method = data[27];
    properties.interlace_method = data[28];
    *bytes_per_pixel = properties.bit_depth / 8;
    properties.width_in_bytes = *width * *bytes_per_pixel;
  }
  else
  {
    // ERROR: IHDR must be the first chunk in a valid PNG file.
  }
  
  int32 chunk_crc = data[32] | (data[31] << 8) | (data[30] << 16) | (data[29] << 24);
  // TODO: Check CRC values
  
  if (*width && *height)
  {
    //image_data = Memory_Allocate(image_data, properties.width_in_bytes * properties.height);
    *texture_data = new byte[properties.width_in_bytes * *height];
  }
  else
  {
    // RETURN: This image is of size 0 by 0
  }
  
  uint32 data_index = 33;
  uint32 extracted_data_index = 0;
  
  //NOTE: Loop through all chunks
  bool end_extraction = false;
  while (!end_extraction)
  {
    chunk_length = (data[data_index + 3]) | (data[data_index + 2] << 8) |
      (data[data_index + 1] << 16) | (data[data_index] << 24);
    chunk_name = data[data_index + 7] | (data[data_index + 6] << 8) |
      (data[data_index + 5] << 16) | (data[data_index + 4] << 24);
    
    data_index += 8;
    
    if (chunk_name == IEND)
      end_extraction = true;
    else
    {
      if (chunk_name == IDAT)
      {
	//NOTE: ZLib Deflate/Inflate Compression information.
	//TODO: Handle more options from just 78 01 (like 78 9C or 78 DA).
	uint8 compression_method = data[data_index++];
	uint8 additional_flags = data[data_index++];
	
	//TODO: Handle the possibility of a DICTID?
	
	//NOTE: index + chunk_lenght  - Two Flag Bytes - 4 ADLER32 Bytes
	uint32 zlib_data_length = data_index + chunk_length - 6;
	
	DeflateBlock block_state;
	block_state.bytes_per_pixel = *bytes_per_pixel;
	block_state.scan_line_pos = properties.width_in_bytes;// Forcing a line reset
	block_state.scan_line_width = properties.width_in_bytes;
	
	//NOTE: Loop through the image data (Deflate compress algorithm)
	while (data_index < zlib_data_length)
	{
	  //NOTE: Block Header
	  /*
	    bit 1: Last block boolean.
	    bits 2-3:  00 - Raw data between 0 and 65,535 bytes in length.
	    01 - Static Huffman compressed block, pree-agreed tree.
	    10 - Compressed block with the Huffman table supplied.
	    11 - Reserved.
	  */
	  uint8 block_header = data[data_index++];

	  uint16 block_length;
	  uint16 block_length_ones_complement;
	  if ((block_header << 7) == 0x00)
	  {
	    block_length = (data[data_index]) | (data[data_index + 1] << 8);
	    data_index += 2;
	    
	    block_length_ones_complement = (data[data_index]) |
	      (data[data_index + 1] << 8);
	    data_index += 2;
	    
	    block_state.length = block_length;
	    block_state.end_index_pos = block_length + data_index;
	    
	    while (data_index < block_state.end_index_pos)
	    {
	      if (block_state.scan_line_pos < block_state.scan_line_width)
	      {
		switch (block_state.current_filter)
		{
		case 0:
		  {
		    no_filter(&data_index, data, &extracted_data_index, *texture_data, &block_state);
		    
		  } break;
		case 1:
		  {
		    sub_filter(&data_index, data, &extracted_data_index, *texture_data, &block_state);
		  } break;
		case 2:
		  {
		    up_filter(&data_index, data, &extracted_data_index, *texture_data, &block_state);
		  } break;
		case 3:
		  {
		    average_filter(&data_index, data, &extracted_data_index, *texture_data, &block_state);
		  } break;
		case 4:
		  {
		    paeth_filter(&data_index, data, &extracted_data_index, *texture_data, &block_state);

		  } break;
		default:
		  {
		    //TODO: Error Handle
		  } break;
		}
		block_state.scan_line_pos++;
		data_index++;
	      }
	      else // NOTE: New scanline
	      {
		block_state.current_filter = data[data_index];
		block_state.scan_line_pos = 0;
		data_index++;
	      }
	    }
	  }
	  else if ((block_header << 7) == 0x80)
	  {
	    block_length = (data[data_index]) | (data[data_index + 1] << 8);
	    data_index += 2;
	    
	    block_length_ones_complement = (data[data_index]) | (data[data_index + 1] << 8);
	    data_index += 2;
	  }
	}
	
	int32 adler_32_checksum = (data[data_index + 3]) | (data[data_index + 2] << 8) |
	  (data[data_index + 1] << 16) | (data[data_index] << 24);
	data_index += 4;
      }
      else if (data_index == 900000)
      {
	break; //TODO: Debug
      }
      else
      {
	//NOTE: Increment the index past the chunk data.
	data_index += chunk_length;
      }
      
      chunk_crc = data[data_index + 3] | (data[data_index + 2] << 8) |
	(data[data_index + 1] << 16) | (data[data_index] << 24);
      data_index += 4;
    }
  }
}
