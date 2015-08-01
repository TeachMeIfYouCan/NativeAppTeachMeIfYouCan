#include <tizen.h>
#include <service_app.h>
#include "zserviceapp.h"

#include <message_port.h>

#define SAMPLE_RATE 48000

#include <audio_io.h>
#include <sound_manager.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

int audio_init();
//void audio_start(const char *remote_app_id);
void audio_in_start();
void audio_out_start(void *b);
void audio_pause();
void audio_stop();
void audio_send_byte();
void send_message(const char * remote_app_id, const char *name);

audio_in_h input; // Declare audio input handle
audio_out_h output; // Declare audio output handle
audio_io_error_e ret;

pthread_t p_thread;
int thread_id;
int thread_exit_signal = 0;

int buffer_size;
void *buffer;

char* remote_app_id_copy = "S8vjRcPYft.zWebApp";

char encoding[256][2];

int audio_init() {

	//ret = audio_in_create(SAMPLE_RATE, AUDIO_CHANNEL_MONO, AUDIO_SAMPLE_TYPE_S16_LE, &input);
	//ret = audio_out_create(SAMPLE_RATE, AUDIO_CHANNEL_MONO, AUDIO_SAMPLE_TYPE_S16_LE, SOUND_TYPE_SYSTEM, &output);

	ret = audio_in_create(SAMPLE_RATE, AUDIO_CHANNEL_MONO, AUDIO_SAMPLE_TYPE_U8, &input);
	ret = audio_out_create(SAMPLE_RATE, AUDIO_CHANNEL_MONO, AUDIO_SAMPLE_TYPE_U8, SOUND_TYPE_SYSTEM, &output);


	ret = audio_in_get_buffer_size(input, &buffer_size);

	if (ret != AUDIO_IO_ERROR_NONE )
		 return -1; // Error handling
	//buffer_size = 1500;
	buffer = malloc(buffer_size);
	memset(buffer, 0, buffer_size);

	dlog_print(DLOG_INFO, "chu", "init audio buffer_size = %d", buffer_size);
	ret = audio_out_prepare(output);

	return 1;
}

char* convert(int ret){
	if(ret >= 256)
		dlog_print(DLOG_INFO, "chu", "convert 256 over");

	return encoding[ret];
}

char deconvert(char* ret){
	int i;

	for(i = 0; i <256; i++) {
		if(ret[0] == encoding[i][0] && ret[1] == encoding[i][1]) {
				return i;
		}
	}

	dlog_print(DLOG_INFO, "chu", "deconvert Abnormal");

	return 0;
}

void audio_in_start() {

	int a = 0, k = 0, i = 0, buffer_len = 0;
	char * temp;
	char * char_buffer;

	while(1){

		ret = audio_in_prepare(input); // Audio input prepare
		ret = audio_in_read(input, buffer, buffer_size);
		ret = audio_in_unprepare(input);

		char * temp_buffer = buffer;

		buffer_len = strlen((char*)temp_buffer);
		dlog_print(DLOG_INFO, "chu_audio_in", "=====================");
		dlog_print(DLOG_INFO, "chu_audio_in", "=====================");

		dlog_print(DLOG_INFO, "chu_audio_in", "bundle buffer                 = %d \t val = %s", buffer_len, temp_buffer);
		char_buffer = (char*)malloc(2 * buffer_size);
		memset(char_buffer, 0, 2 * buffer_size);

		/*
		int * int_buffer = (int *)buffer;
		for(i = 0; i < buffer_len; i++) {
			dlog_print(DLOG_INFO, "chu_audio_in", "i = %d int_buffer[%d] = %d " , i, i, int_buffer[i]);
		}
		*/

		for(i = 0; i < buffer_len; i++) {
			//memset(temp, 0, sizeof(temp));
			//dlog_print(DLOG_INFO, "chu", "i = %d int_buffer[i] = %d", i , temp_buffer[i]);

			temp = convert(temp_buffer[i]);
			//dlog_print(DLOG_INFO, "chu", "bundle temp value =  %s", temp);

			char_buffer[k++]  = temp[0];
			char_buffer[k++]  = temp[1];
			//memset(temp, 0, sizeof(temp));
		}


		dlog_print(DLOG_INFO, "chu_audio_in", "bundle char_buffer           = %d \t val = %s", strlen(char_buffer), char_buffer);

		audio_send_byte(char_buffer);

		free(char_buffer);

		k = 0;
		a++;
		if(a==20){ break; }
	}

}

void audio_out_start(void *b) {

	ret = audio_out_write(output, buffer, buffer_size);
}

void audio_send_byte(char* char_buffer){

	bundle *b = bundle_create();

	//dlog_print(DLOG_INFO, "chu", "bundle add before");

	bundle_add_str(b, "foo", char_buffer);

	//To send a message through a specific message port
	message_port_send_message(remote_app_id_copy, "RECEIVE_HELLO_MESSAGE", b);

	//dlog_print(DLOG_INFO, "chu", "message send");

	bundle_free(b);
}

void send_message(const char * remote_app_id, const char *name){
	/*
	bundle *b = bundle_create();
	char hello_message[20] = "Hello ";
	strcat(hello_message, name);
	bundle_add_str(b, "helloMessage", hello_message);

	//To send a message through a specific message port
	//message_port_send_message(remote_app_id, "RECEIVE_HELLO_MESSAGE", b);
	//원격으로 보내는 포트의 메시지의 이름이 "RECEIVE_HELLO_MESSAGE" 이고, b가 "Hello " + name("Web")을 해서 보냄
	bundle_free(b);
	*/
}

void message_port_cb(int local_port_id, const char *remote_app_id,
		const char *remote_port, bool trusted_remote_port, bundle *message) {

	char *name = NULL;
	bundle_get_str(message, "command", &name); //Web에서 "name"이라는 키를 받고  name변수에 값인 "Web"을 저장

	//To print received message
	dlog_print(DLOG_INFO, "chu message receive", "Message from %s, name: %s ",
	 remote_app_id, name);


	if(strcmp(name, "audio_start") == 0) {
		thread_id = pthread_create(&p_thread, NULL, audio_in_start, NULL);
		pthread_detach(p_thread);
		//dlog_print(DLOG_INFO, "chu message receive", "thread start end");
	}
	else if(strcmp(name, "audio_pause") == 0) {

	}
	else if(strcmp(name, "audio_stop") == 0) {

	}

	//To send a message through a specific message port
	//send_message(remote_app_id, name);
}

void audio_message_port_cb(int local_port_id, const char *remote_app_id,
		const char *remote_port, bool trusted_remote_port, bundle *message) {
	int i, buffer_len;
	char * char_buffer;
	char * res_buffer;

	//memset(void_buffer, 0, sizeof(void_buffer));

	bundle_get_str(message, "foo", &char_buffer);

	buffer_len = strlen(char_buffer);

	dlog_print(DLOG_INFO, "chu msg_recv", "msg_port char_buffer        = %d \t val = %s", buffer_len, char_buffer);

	res_buffer = (char*)malloc(buffer_size);
	memset(res_buffer, 0, buffer_size);

	if(buffer_len != 0) {
		for(i = 0; i < buffer_len / 2; i++) {
			char temp[2];
			char _temp;
			temp[0] = char_buffer[i * 2];
			temp[1] = char_buffer[i * 2 + 1];

			//memset(temp, 0, sizeof(temp));
			//dlog_print(DLOG_INFO, "chu", "i = %d int_buffer[i] = %d", i , temp_buffer[i]);

			_temp = deconvert(temp);
			//dlog_print(DLOG_INFO, "chu", "bundle temp value =  %s", temp);

			res_buffer[i] = _temp;
			//memset(temp, 0, sizeof(temp));
		}
	}

	dlog_print(DLOG_INFO, "chu_msg_recv", "msg_port res_buffer          = %d \t val = %s", strlen(res_buffer), res_buffer);

	ret = audio_out_write(output, (void*)res_buffer, buffer_size);
	free(res_buffer);
}


bool service_app_create(void *data)
{
    // Todo: add your code here.

	int i , j, k = 0;

	dlog_print(DLOG_INFO, "NativeService", "Created.");

	audio_init();

	int local_port_id = message_port_register_local_port("COMMAND_VALUE", message_port_cb, NULL);
	int local_port_id2 = message_port_register_local_port("RECEIVE_AUDIO", audio_message_port_cb, NULL);

	if (local_port_id < 0)
		dlog_print(DLOG_ERROR, "NativeService", "Port register error : %d", local_port_id);
	else
		dlog_print(DLOG_INFO, "NativeService", "port_id : %d", local_port_id);

	for (i = 'a'; i <= 'z'; i++){

		for (j = 'a'; j<='z'; j++){

			encoding[k][0] = i;
			encoding[k][1] = j;

			k++;
			if (k == 256)
				break;
		}
		if (k == 256)
			break;
	}
    return true;
}
void service_pause(void *data)
{
	dlog_print(DLOG_INFO, "chu", "service_pause called");
	free(buffer);

	ret = audio_in_unprepare(input);
    ret = audio_in_destroy(input);
    ret = audio_out_unprepare(output);
    ret = audio_out_destroy(output);

}

void service_app_resume(void *data)
{
    // Todo: add your code here.
	dlog_print(DLOG_INFO, "chu", "service_app_resume called");

	audio_init();
}

void service_app_terminate(void *data)
{
    // Todo: add your code here.
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
    // Todo: add your code here.
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
