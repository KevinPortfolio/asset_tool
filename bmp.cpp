#define WINDOWS_TYPE 16973 // NOTE: 0x42 0x4D

/*
  - Bitmap file header: 
    14 bytes in size. 
  - DIB header
  - Extra bit masks (optional)
  - Color table (partially optional)
  - Gap1 (optional)
  - Pixel array:
    Must begin at an address that is a multiple of 4 bytes.
  - Gap2 (optional)
  - ICC Color Profile (optional)

 */

struct bmp_file
{
  byte* data;
  int32 width;
  int32 height;
  int8 bytes_per_pixel;
};

static bmp_file
bmp_extract(byte* data_input)
{
  bmp_file result = {};
  uint16 header_type = (data_input[1]) | (data_input[0] << 8);
 
  uint32 file_size = (data_input[2]) | (data_input[3] << 8) | (data_input[4] << 16) |
    (data_input[5] << 24);
  uint32 pixel_array_offset = (data_input[10]) | (data_input[11] << 8) |
    (data_input[12] << 16) | (data_input[13] << 24);
  
  if (header_type == WINDOWS_TYPE)
  {
    uint32 header_size = (data_input[14]) | (data_input[15] << 8) | (data_input[16] << 16) |
      (data_input[17] << 24);
    
    result.width = (data_input[18]) | (data_input[19] << 8) |
      (data_input[20] << 16) | (data_input[21] << 24);
    result.height = (data_input[22]) | (data_input[23] << 8) |
      (data_input[24] << 16) | (data_input[25] << 24);
    
    uint16 bits_per_pixel = (data_input[28]) | (data_input[29] << 8);
    result.bytes_per_pixel = bits_per_pixel / 8;
    
    uint32 compression_method = (data_input[30]) | (data_input[31] << 8) |
      (data_input[32] << 16) | (data_input[33] << 24);
    
    uint32 image_size = (data_input[34]) | (data_input[35] << 8) | (data_input[36] << 16) |
      (data_input[37] << 24);

    // NOTE: Why is bits per pixel here?
    uint32 bytes_per_row = ((bits_per_pixel * result.width + 31) / 32) * 4;
    uint32 padding_offset = bytes_per_row - (result.width * result.bytes_per_pixel);

    // NOTE: Is image_size not required on a valid bmp file?
    uint32 size_check = result.width * result.height * result.bytes_per_pixel +
	(padding_offset * result.height); 
    if (image_size == 0)
    {
      image_size = size_check; // NOTE: Should this be an error?
    }
    if (image_size != size_check)
    {
      result.width = 0;
      result.height = 0;
      return result;
    }
    
    uint32 horizontal_resolution  = (data_input[38]) | (data_input[39] << 8) |
      (data_input[40] << 16) | (data_input[41] << 24);
    uint32 vertical_resolution = (data_input[42]) | (data_input[43] << 8) |
      (data_input[44] << 16) | (data_input[45] << 24);
    uint32 colors_in_pallet_count = (data_input[46]) | (data_input[47] << 8) |
      (data_input[48] << 16) | (data_input[49] << 24);
    uint32 important_colors_count = (data_input[50]) | (data_input[51] << 8) |
      (data_input[52] << 16) | (data_input[53] << 24);
    
    if ((compression_method == 0) && (colors_in_pallet_count == 0))
    {
      result.data = new byte[image_size + 1]{};
      result.data[image_size] = 0;
      uint32 image_data_index = 0;
      uint32 pixel_row_index = 0;
      uint32 byte_width_index = 0;
      uint32 pixel_column_index = 0;
      
      if (result.bytes_per_pixel == 3)
      {
	for (pixel_row_index; pixel_row_index < result.height; ++pixel_row_index)
	{

	  pixel_column_index = 0;
	  for (pixel_column_index; pixel_column_index < result.width; ++pixel_column_index)
	  {
	    
	    if ((pixel_array_offset + (pixel_row_index * bytes_per_row) +
		 byte_width_index + 2) > file_size)
	    {
	      // TODO: Error
	    }

	    result.data[image_data_index + 0] =
	      data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
						    byte_width_index + 2];
	    result.data[image_data_index + 1] =
	      data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
						    byte_width_index + 1];
	    result.data[image_data_index + 2] =
	      data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
						    byte_width_index + 0];
	    
	    byte_width_index += result.bytes_per_pixel;
	    image_data_index += result.bytes_per_pixel;
	  }
	}
      }
      else if (result.bytes_per_pixel == 4)
      {
	
	for (pixel_row_index; pixel_row_index < result.height; ++pixel_row_index)
	{

	  pixel_column_index = 0;
	  for (pixel_column_index; pixel_column_index < result.width; ++pixel_column_index)
	  {

	    if ((pixel_array_offset + (pixel_row_index * bytes_per_row) +
		 byte_width_index + 3) > file_size)
	    {
	      // TODO: Error
	    }

	    result.data[image_data_index + 0] =
	      data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
			 byte_width_index + 2];
	    result.data[image_data_index + 1] =
	      data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
			 byte_width_index + 1];
	    result.data[image_data_index + 2] =
	      data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
			 byte_width_index + 0];
	    result.data[image_data_index + 3] =
	      0XFF + data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
				byte_width_index + 3];
	    
	    byte_width_index += result.bytes_per_pixel;
	    image_data_index += result.bytes_per_pixel;
	  }
	}
      }
      else
      {
	// TODO: Error
      }
    }
    else
    {
      // TODO: Error
    }
  }
}
