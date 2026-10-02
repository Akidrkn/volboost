#include <stdio.h>
#include <stdlib.h>
#include <pulse/pulseaudio.h>

pa_mainloop *mainloop = NULL;
double volume_percent = 100.0;

void context_state_callback(pa_context *context, void *userdata);
void server_info_callback(pa_context *context, const pa_server_info *info, void *userdata);
void sink_info_callback(pa_context *context, const pa_sink_info *info, int eol, void *userdata);
void volume_set_callback(pa_context *context, int success, void *userdata);


int main(void)
{
    printf("Укажите громкость в процентах: ");

    if (scanf("%lf", &volume_percent) != 1)
    {
        printf("Ошибка: нужно ввести число.\n");
        return 1;
    }

    if (volume_percent < 0)
    {
        printf("Ошибка: громкость не может быть отрицательной.\n");
        return 1;
    }


    mainloop = pa_mainloop_new();

    if (mainloop == NULL)
    {
        printf("Не удалось создать PulseAudio mainloop.\n");
        return 1;
    }


    pa_mainloop_api *mainloop_api;

    mainloop_api = pa_mainloop_get_api(mainloop);


    pa_context *context;

    context = pa_context_new(mainloop_api, "volume_control");

    if (context == NULL)
    {
        printf("Не удалось создать PulseAudio context.\n");

        pa_mainloop_free(mainloop);
        return 1;
    }


    pa_context_set_state_callback(
        context,
        context_state_callback,
        NULL
    );


    if (pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0)
    {
        printf("Ошибка подключения: %s\n",
               pa_strerror(pa_context_errno(context)));

        pa_context_unref(context);
        pa_mainloop_free(mainloop);

        return 1;
    }


    int ret = 0;

    pa_mainloop_run(mainloop, &ret);


    pa_context_disconnect(context);
    pa_context_unref(context);

    pa_mainloop_free(mainloop);


    return ret;
}


void context_state_callback(pa_context *context, void *userdata)
{
    pa_context_state_t state;

    state = pa_context_get_state(context);


    if (state == PA_CONTEXT_READY)
    {
        printf("Подключение к PulseAudio успешно.\n");


        pa_operation *operation;

        operation = pa_context_get_server_info(
            context,
            server_info_callback,
            NULL
        );


        if (operation != NULL)
        {
            pa_operation_unref(operation);
        }
    }

    else if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED)
    {
        printf("PulseAudio: %s\n",
               pa_strerror(pa_context_errno(context)));


        pa_mainloop_quit(mainloop, 1);
    }
}


void server_info_callback(pa_context *context, const pa_server_info *info, void *userdata)
{
    if (info == NULL || info->default_sink_name == NULL)
    {
        printf("Не удалось определить аудиовыход по умолчанию.\n");

        pa_mainloop_quit(mainloop, 1);

        return;
    }


    printf("Аудиовыход: %s\n", info->default_sink_name);


    pa_operation *operation;

    operation = pa_context_get_sink_info_by_name(
        context,
        info->default_sink_name,
        sink_info_callback,
        NULL
    );


    if (operation != NULL)
    {
        pa_operation_unref(operation);
    }
}


void sink_info_callback(pa_context *context, const pa_sink_info *info, int eol, void *userdata)
{
    if (eol != 0)
    {
        return;
    }


    if (info == NULL)
    {
        printf("Не удалось получить информацию о sink.\n");

        pa_mainloop_quit(mainloop, 1);

        return;
    }


    unsigned int num_channels;

    num_channels = info->channel_map.channels;


    printf("Количество каналов: %u\n", num_channels);


    double multiplier;

    multiplier = volume_percent / 100.0;


    double raw_volume;

    raw_volume = (double)PA_VOLUME_NORM * multiplier;


    if (raw_volume > PA_VOLUME_MAX)
    {
        raw_volume = PA_VOLUME_MAX;
    }


    pa_volume_t volume;

    volume = (pa_volume_t)raw_volume;


    pa_cvolume cv;

    pa_cvolume_set(&cv, num_channels, volume);




    pa_operation *operation;

    operation = pa_context_set_sink_volume_by_name(
        context,
        info->name,
        &cv,
        volume_set_callback,
        NULL
    );


    if (operation == NULL)
    {
        printf("Не удалось создать операцию изменения громкости.\n");

        pa_mainloop_quit(mainloop, 1);

        return;
    }


    pa_operation_unref(operation);
}


void volume_set_callback(pa_context *context, int success, void *userdata)
{
    if (success)
    {
        printf("Громкость успешно установлена на %.1f%%\n", volume_percent);


        pa_mainloop_quit(mainloop, 0);
    }
    else
    {
        printf("Не удалось установить громкость: %s\n",
               pa_strerror(pa_context_errno(context)));


        pa_mainloop_quit(mainloop, 1);
    }
}