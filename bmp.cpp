#define WINDOWS_TYPE 16973 // NOTE: 0x42 0x4D

static void
bmp_extract(byte* data_input, byte** data_output, uint32* img_width, uint32* img_height,
		     uint8* bytes_per_pixel)
{
  uint16 header_type = (data_input[1]) | (data_input[0] << 8);
  uint32 file_size = (data_input[2]) | (data_input[3] << 8) | (data_input[4] << 16) | (data_input[5] << 24);
  
  uint32 pixel_array_offset = (data_input[10]) | (data_input[11] << 8) | (data_input[12] << 16) |
    (data_input[13] << 24);
  
  if (header_type == WINDOWS_TYPE)
  {
    uint32 header_size = (data_input[14]) | (data_input[15] << 8) | (data_input[16] << 16) |
      (data_input[17] << 24);
    
    *img_width = (data_input[18]) | (data_input[19] << 8) | (data_input[20] << 16) | (data_input[21] << 24);
    *img_height = (data_input[22]) | (data_input[23] << 8) | (data_input[24] << 16) | (data_input[25] << 24);
    
    uint32 image_area = *img_width * *img_height;
    
    uint16 bits_per_pixel = (data_input[28]) | (data_input[29] << 8);
    *bytes_per_pixel = bits_per_pixel / 8;
    
    uint32 compression_method = (data_input[30]) | (data_input[31] << 8) | (data_input[32] << 16) |
      (data_input[33] << 24);
    
    uint32 image_size = (data_input[34]) | (data_input[35] << 8) | (data_input[36] << 16) | (data_input[37] << 24);
    
    uint32 bytes_per_row = ((bits_per_pixel * *img_width + 31) / 32) * 4;
    uint32 padding_offset = bytes_per_row - (*img_width * *bytes_per_pixel);
    
    if (image_size == 0)
      image_size = image_area * *bytes_per_pixel + (padding_offset * *img_height);
    
    uint32 horizontal_resolution  = (data_input[38]) | (data_input[39] << 8) | (data_input[40] << 16) |
      (data_input[41] << 24);
    
    uint32 vertical_resolution = (data_input[42]) | (data_input[43] << 8) | (data_input[44] << 16) |
      (data_input[45] << 24);
    
    uint32 colors_in_pallet_count = (data_input[46]) | (data_input[47] << 8) | (data_input[48] << 16) |
      (data_input[49] << 24);
    
    uint32 important_colors_count = (data_input[50]) | (data_input[51] << 8) | (data_input[52] << 16) |
      (data_input[53] << 24);
    
    if ((compression_method == 0) && (colors_in_pallet_count == 0))
    {
      byte* image_data = new byte[image_size + 1]{};
      uint32 image_data_index = 0;
      
      if (*bytes_per_pixel == 3)
      {
	for (uint32 pixel_row_index = 0; pixel_row_index < *img_height; pixel_row_index++)
	{
	  
	  uint32 byte_width_index = 0;
	  for (uint32 pixel_column_index = 0; pixel_column_index < *img_width; pixel_column_index++)
	  {
	    if ((pixel_array_offset + (pixel_row_index * bytes_per_row) + byte_width_index + 2) > file_size)
	    {
	      // TODO: Error
	    }

	    image_data[image_data_index + 0] = data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
						    byte_width_index + 2];
	    image_data[image_data_index + 1] = data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
						    byte_width_index + 1];
	    image_data[image_data_index + 2] = data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
						    byte_width_index + 0];
	    
	    byte_width_index += *bytes_per_pixel;
	    image_data_index += *bytes_per_pixel;
	  }
	}
      }
      else if (*bytes_per_pixel == 4)
      {
	
	for (uint32 pixel_row_index = 0; pixel_row_index < *img_height; pixel_row_index++)
	{
	  uint32 byte_width_index = 0;
	  for (uint32 pixel_column_index = 0; pixel_column_index < *img_width; pixel_column_index++)
	  {

	    if ((pixel_array_offset + (pixel_row_index * bytes_per_row) + byte_width_index + 3) > file_size)
	    {
	      // TODO: Error
	    }

	    image_data[image_data_index + 0] = data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
						    byte_width_index + 2];
	    image_data[image_data_index + 1] = data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
						    byte_width_index + 1];
	    image_data[image_data_index + 2] = data_input[pixel_array_offset + (pixel_row_index * bytes_per_row) +
						    byte_width_index + 0];
	    image_data[image_data_index + 3] = 0XFF + data_input[pixel_array_offset +
						    (pixel_row_index * bytes_per_row) + byte_width_index + 3];
	    
	    byte_width_index += *bytes_per_pixel;
	    image_data_index += *bytes_per_pixel;
	  }
	}
      }
      else
      {
	// TODO: Error
      }
      
      *data_output = image_data;
    }
    else
    {
      // TODO: Error
    }
  }
}
