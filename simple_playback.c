#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <stdint.h>

// Structure to hold the metadata for writing wav files (to store the output)
typedef struct WaveHeader 
{
    char RIFF_marker[4];
    uint32_t file_size;
    char filetype_header[4];
    char format_marker[4];
    uint32_t data_header_length;
    uint16_t format_type;
    uint16_t number_of_channels;
    uint32_t sample_rate;
    uint32_t bytes_per_second;
    uint16_t bytes_per_frame;
    uint16_t bits_per_sample;
} wav_header;

// Debug function to view contents of some wav header structure
void * print_wav_header(wav_header * hdr) {
	printf("--- WAV HEADER ---");
	printf("RIFF marker  : %s\n", hdr->RIFF_marker);
	printf("File size    : %d\n", hdr->file_size);
	printf("File type    : %s\n", hdr->filetype_header);
	printf("Format marker: %s\n", hdr->format_marker);
	printf("Data length  : %d\n", hdr->data_header_length);
	printf("Format type  : %d\n", hdr->format_type);
	printf("Channels     : %d\n", hdr->number_of_channels);
	printf("Sample rate  : %d\n", hdr->sample_rate);
	printf("Bytes / sec  : %d\n", hdr->bytes_per_second);
	printf("Bytes / Frame: %d\n", hdr->bytes_per_frame);
	printf("Bits / Sample: %d\n", hdr->bits_per_sample);
}

int main(int argc, char ** argv) {
    
    // Variable declaration
	int rc;
	char * buffer;
  	int buffer_size;
  	int periods_per_buffer;

  	snd_pcm_t *handle;
  	snd_pcm_hw_params_t *params;
  	snd_pcm_uframes_t frames;

  	unsigned int channels;
  	unsigned int rate;

	wav_header * wav_header_info;

  	FILE * fp;

    
    // Argument parsing
  	if (argc != 2)
	{
        printf("Incorrect usage: Enter filename of the wav file you want to play as an argument: %s filename.txt\n", argv[0]);
		return 1;
	}

    
    // Open wav file to read
	fp = fopen(argv[1], "rb");

	if (fp == NULL)
	{
		printf("ERROR: %s does not exist, or cannot be opened.\n", argv[1]);
		return 1;
	}

	wav_header_info = malloc(44);

	fread(wav_header_info, 1, 44, fp);
    

	// print_wav_header(wav_header_info);

    
    // Assign variables that were read from the wave file
	channels = wav_header_info->number_of_channels;
	rate = wav_header_info->sample_rate;
	periods_per_buffer = 2; // Down to user preference, depending on size of internal ring buffer of ALSA


  	// Open PCM device for playback
  	if ((rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) 
  	{
    	printf("ERROR: Cannot open pcm device. %s\n", snd_strerror(rc));
  	}

    
  	// Allocate hardware parameters
  	if ((rc = snd_pcm_hw_params_malloc(&params)) < 0)
  	{
  		printf("ERROR: Cannot allocate hardware parameters. %s\n", snd_strerror(rc));
  	}

    
  	// Initialize parameters with default values
  	if ((rc = snd_pcm_hw_params_any(handle, params)) < 0)
  	{
  		printf("ERROR: Cannot initialize hardware parameters. %s\n", snd_strerror(rc));
  	}


  	// Setting hardware parameters
  	if ((rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
  	{
  		printf("ERROR: Cannot set interleaved mode. %s\n", snd_strerror(rc));
  	}

  	if ((rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE)) < 0)
  	{
  		printf("ERROR: Cannot set PCM format. %s\n", snd_strerror(rc));
  	}

  	if ((rc = snd_pcm_hw_params_set_channels_near(handle, params, &channels)) < 0)
  	{
  		printf("ERROR: Cannot set number of channels. %s\n", snd_strerror(rc));
  	}

 	if ((rc = snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0)) < 0)
 	{
 		printf("ERROR: Cannot set plyabck rate. %s\n", snd_strerror(rc));
 	}

 	if ((rc = snd_pcm_hw_params(handle, params)) < 0)
 	{
 		printf("ERROR: Cannot set hardware parameters. %s\n", snd_strerror(rc));
 	}


 	// Get hardware parameters
 	if ((rc = snd_pcm_hw_params_get_period_size(params, &frames, 0)) < 0)
	{
		printf("Playback ERROR: Can't get period size. %s\n", snd_strerror(rc));
	}
	printf("Frames: %lu\n", frames);

	if ((rc = snd_pcm_hw_params_get_channels(params, &channels)) < 0)
	{
		printf("Playback ERROR: Can't get channel number. %s\n", snd_strerror(rc));
	}

	if ((rc = snd_pcm_hw_params_get_rate(params, &rate, 0)) < 0)
	{
		printf("ERROR: Cannot get rate. %s\n", snd_strerror(rc));
	}


	// Free paraemeters
	snd_pcm_hw_params_free(params);


	// Create buffer
  	buffer_size = frames * periods_per_buffer * channels * sizeof(int16_t); /* 2 bytes/sample, 2 channels */
  	buffer = (char *) malloc(buffer_size);

  	// Send info to ALSA
 	//while ((rc = read(0, buffer, buffer_size)) != 0) 
 	while (rc = fread(buffer, 1, periods_per_buffer * frames * channels * sizeof(int16_t), fp) != 0)
 	{
    	rc = snd_pcm_writei(handle, buffer, frames * periods_per_buffer);
    	if (rc == -EPIPE) 
    	{
      		fprintf(stderr, "underrun occurred\n");
      		snd_pcm_prepare(handle);
    	} 
    	else if (rc < 0) 
    	{
      		printf("ERROR: Cannot write to playback device. %s\n", strerror(rc));
    	}
  	}

  	printf("Info set: Device is now draining...\n");
  	snd_pcm_drain(handle);

  	printf("Done playing, closing connections.\n");
  	snd_pcm_close(handle);

  	free(wav_header_info);
  	free(buffer);
  	fclose(fp);

  return 0;
}
