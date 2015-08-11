#include <tizen.h>
#include <service_app.h>
#include "zserviceapp.h"

#include <message_port.h>

#define SAMPLE_RATE 8000

#include <audio_io.h>
#include <sound_manager.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

int audio_init();
void convert_init();
char* convert(int ret);
char deconvert(char* ret);
void* audio_in_start();
void audio_send_byte(char* char_buffer);


//char* remote_app_id_copy = "S8vjRcPYft.zWebApp";

char* remote_app_id_copy= "Y2QbwJHU6E.TizenProjectTeachMeIFYouCanVER01";

char encoding[256][2];

pthread_t p_thread;
int audio_signal_check = 0;
int audio_exit_check = 0;

audio_in_h input; // Declare audio input handle
audio_out_h output; // Declare audio output handle
audio_io_error_e ret;

int buffer_size = 512;
void* buffer;

/* ---- Base64 Encoding/Decoding Table --- */
char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* decodeblock - decode 4 '6-bit' characters into 3 8-bit binary bytes */
void decodeblock(unsigned char in[], char *clrstr) {
  unsigned char out[4];
  out[0] = in[0] << 2 | in[1] >> 4;
  out[1] = in[1] << 4 | in[2] >> 2;
  out[2] = in[2] << 6 | in[3] >> 0;
  out[3] = '\0';
  strncat(clrstr, out, sizeof(out));
}

void b64_decode(char *b64src, char *clrdst) {
  int c, phase, i;
  unsigned char in[4];
  char *p;

  clrdst[0] = '\0';
  phase = 0; i=0;
  while(b64src[i]) {
    c = (int) b64src[i];
    if(c == '=') {
      decodeblock(in, clrdst);
      break;
    }
    p = strchr(b64, c);
    if(p) {
      in[phase] = p - b64;
      phase = (phase + 1) % 4;
      if(phase == 0) {
        decodeblock(in, clrdst);
        in[0]=in[1]=in[2]=in[3]=0;
      }
    }
    i++;
  }
}

/* encodeblock - encode 3 8-bit binary bytes as 4 '6-bit' characters */
void encodeblock( unsigned char in[], char b64str[], int len ) {
    unsigned char out[5];
    out[0] = b64[ in[0] >> 2 ];
    out[1] = b64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? b64[ ((in[1] & 0x0f) << 2) |
             ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? b64[ in[2] & 0x3f ] : '=');
    out[4] = '\0';
    strncat(b64str, out, sizeof(out));
}

/* encode - base64 encode a stream, adding padding if needed */
void b64_encode(char *clrstr, char *b64dst) {
  unsigned char in[3];
  int i, len = 0;
  int j = 0;

  b64dst[0] = '\0';
  while(clrstr[j]) {
    len = 0;
    for(i=0; i<3; i++) {
     in[i] = (unsigned char) clrstr[j];
     if(clrstr[j]) {
        len++; j++;
      }
      else in[i] = 0;
    }
    if( len ) {
      encodeblock( in, b64dst, len );
    }
  }
}

int audio_init() {

	//ret = audio_in_create(SAMPLE_RATE, AUDIO_CHANNEL_MONO, AUDIO_SAMPLE_TYPE_S16_LE, &input);
	//ret = audio_out_create(SAMPLE_RATE, AUDIO_CHANNEL_MONO, AUDIO_SAMPLE_TYPE_S16_LE, SOUND_TYPE_SYSTEM, &output);

	ret = audio_in_create(SAMPLE_RATE, AUDIO_CHANNEL_STEREO, AUDIO_SAMPLE_TYPE_U8, &input);
	ret = audio_out_create(SAMPLE_RATE, AUDIO_CHANNEL_STEREO, AUDIO_SAMPLE_TYPE_U8, SOUND_TYPE_SYSTEM, &output);


	//ret = audio_in_get_buffer_size(input, &buffer_size);

	if (ret != AUDIO_IO_ERROR_NONE )
	{
		dlog_print(DLOG_INFO, "chu", "audio_in_create err");
		return -1; // Error handling
	}

	buffer = malloc(buffer_size);
	memset(buffer, 0, buffer_size);

	dlog_print(DLOG_INFO, "chu", "audio_init called buffer_size = %d SAMPLE_RATE = %d", buffer_size, SAMPLE_RATE);

	return 1;
}

void convert_init() {
	int k = 0;

	for (int i = 'a'; i <= 'z'; i++){

		for (int j = 'a'; j<='z'; j++){

			encoding[k][0] = i;
			encoding[k][1] = j;

			k++;
			if (k == 256)
				break;
		}
		if (k == 256)
			break;
	}
}

char* convert(int ret){
	if(ret >= 256)
		dlog_print(DLOG_INFO, "chu", "convert 256 over");

	return encoding[ret];
}

char deconvert(char* ret){
	for(int i = 0; i <256; i++)
		if(ret[0] == encoding[i][0] && ret[1] == encoding[i][1])
			return i;

	dlog_print(DLOG_INFO, "chu", "deconvert Abnormal");
	return '@';
}

void* audio_in_start() {

	int k = 0;

	while(1) {
		while(audio_signal_check == 0) {

			ret = audio_in_prepare(input); // Audio input prepare
			ret = audio_in_read(input, buffer, buffer_size);
			ret = audio_in_unprepare(input);

			char* temp_buffer = buffer;

			//char* send_buffer = (char*)malloc(2 * buffer_size);
			//memset(send_buffer, 0, 2 * buffer_size);
			//memset(send_buffer, 0, 2 * buffer_size);

			//char* send_buffer;
			char send_buffer[512];
			//memset(send_buffer, 0, buffer_size);

			//size_t olen = buffer_size;
			//size_t buSize = buffer_size;

			dlog_print(DLOG_INFO, "chu_audio_in", "==================");
			dlog_print(DLOG_INFO, "chu_audio_in", "==================");

			b64_encode(temp_buffer, send_buffer);
			/*

			if(mbedtls_base64_encode( temp_buffer, buSize, &olen, buffer, buffer_size ) == 0)
					dlog_print(DLOG_INFO, "chu_audio_in", "tmbedtls_base64_encode success");
			*/

			/*
			for(int i = 0; i < 10; i++) {
				dlog_print(DLOG_INFO, "chu_audio_in", "atfer send_buffer[%d] = %c", i, send_buffer[i]);
			}
			 */


			/*
			for(int i = 0; i < buffer_size; i++) {
				char* temp = convert(temp_buffer[i]);
				send_buffer[k++]  = temp[0];
				send_buffer[k++]  = temp[1];
			}

			//dlog_print(DLOG_INFO, "chu_audio_in", "=====================================================================");
		    //dlog_print(DLOG_INFO, "chu_audio_in", "temp_buffer len = %d val = %s", strlen(temp_buffer), temp_buffer);
			//dlog_print(DLOG_INFO, "chu_audio_in", "send_buffer len = %d val = %s", strlen(send_buffer), send_buffer);
			*/
			audio_send_byte(send_buffer);

			//free(send_buffer);
			k = 0;

			if(audio_exit_check == 1)
				break;
		}

		if(audio_exit_check == 1) {
			audio_exit_check = 0;
			break;
		}
	}

	return (void*)NULL;
}

void audio_send_byte(char* char_buffer){

	bundle *b = bundle_create();

	bundle_add_str(b, "foo", char_buffer);

	message_port_send_message(remote_app_id_copy, "RECEIVE_HELLO_MESSAGE", b);

	bundle_free(b);
}

void message_port_cb(int local_port_id, const char *remote_app_id,
		const char *remote_port, bool trusted_remote_port, bundle *message) {

	char *name = NULL;
	bundle_get_str(message, "command", &name); //Web에서 "name"이라는 키를 받고  name변수에 값인 "Web"을 저장

	//To print received message
	//dlog_print(DLOG_INFO, "chu message receive", "Message from %s, name: %s ", remote_app_id, name);

	if(strcmp(name, "audio_start") == 0) {
		dlog_print(DLOG_INFO, "chu message receive", "command audio_start");
		if(audio_signal_check == 0) {
			pthread_create(&p_thread, NULL, audio_in_start, NULL);
			pthread_detach(p_thread);
			dlog_print(DLOG_INFO, "chu message receive", "command pthread create");
		}
		audio_signal_check = 0;
		//dlog_print(DLOG_INFO, "chu message receive", "command audio_signal_check = %d audio_exit_check = %d", audio_signal_check, audio_exit_check );
	}
	else if(strcmp(name, "audio_pause") == 0) {
		dlog_print(DLOG_INFO, "chu message receive", "command audio_pause");
		audio_signal_check = 1;
		//dlog_print(DLOG_INFO, "chu message receive", "command audio_signal_check = %d audio_exit_check = %d", audio_signal_check, audio_exit_check );
	}
	else if(strcmp(name, "audio_stop") == 0) {
		dlog_print(DLOG_INFO, "chu message receive", "command audio_stop");
		audio_signal_check = 1;
		//audio_signal_check = 0;
		//audio_exit_check = 1;
		//dlog_print(DLOG_INFO, "chu message receive", "command audio_signal_check = %d audio_exit_check = %d", audio_signal_check, audio_exit_check );
	}
}

void audio_message_port_cb(int local_port_id, const char *remote_app_id,
		const char *remote_port, bool trusted_remote_port, bundle *message, void* n) {

	char * recv_buffer;
	char res_buffer[512];
	memset(res_buffer, 0, buffer_size);

	bundle_get_str(message, "foo", &recv_buffer);

	b64_decode(recv_buffer, res_buffer);

	/*
	if(strlen(recv_buffer) > 1) {
		for(int i = 0; i < buffer_size; i++) {
			char temp[2];
			char _temp;
			temp[0] = recv_buffer[i * 2];
			temp[1] = recv_buffer[i * 2 + 1];

			_temp = deconvert(temp);

			res_buffer[i] = _temp;
		}
	}
	*/
	//dlog_print(DLOG_INFO, "chu_audio_out", "recv_buffer len = %d val = %s", strlen(recv_buffer), recv_buffer);
	dlog_print(DLOG_INFO, "chu_audio_out", "res_buffer  len = %d val = %s", strlen(res_buffer), res_buffer);

	ret = audio_out_prepare(output);
	ret = audio_out_write(output, (void*)res_buffer, buffer_size);
	ret = audio_out_unprepare(output);

	//free(res_buffer);
}


bool service_app_create(void *data)
{
	dlog_print(DLOG_INFO, "NativeService", "Created.");

	audio_init();
	convert_init();

	int local_port_id = message_port_register_local_port("COMMAND_VALUE", message_port_cb, NULL);
	int local_port_id2 = message_port_register_local_port("RECEIVE_AUDIO", audio_message_port_cb, NULL);

	if (local_port_id < 0)
		dlog_print(DLOG_ERROR, "NativeService", "COMMAND_VALUE Port register error : %d", local_port_id);
	else
		dlog_print(DLOG_INFO, "NativeService", "COMMAND_VALUE port_id : %d", local_port_id);

	if (local_port_id2 < 0)
			dlog_print(DLOG_ERROR, "NativeService", "RECEIVE_AUDIO Port register error : %d", local_port_id2);
		else
			dlog_print(DLOG_INFO, "NativeService", "RECEIVE_AUDIO port_id : %d", local_port_id2);

    return true;
}

void service_app_terminate(void *data)
{
	dlog_print(DLOG_INFO, "chu", "service_app_terminate called");

	free(buffer);

	ret = audio_in_unprepare(input);
    ret = audio_in_destroy(input);
    ret = audio_out_unprepare(output);
    ret = audio_out_destroy(output);

    return;
}

void service_app_control(app_control_h app_control, void *data)
{

}

static void
service_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	return;
}

static void
service_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
service_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
service_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
service_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int main(int argc, char* argv[])
{
    char ad[50] = {0,};
	service_app_lifecycle_callback_s event_callback;
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	service_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, service_app_low_battery, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, service_app_low_memory, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, service_app_orient_changed, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, service_app_lang_changed, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, service_app_region_changed, &ad);

	return service_app_main(argc, argv, &event_callback, ad);
}
