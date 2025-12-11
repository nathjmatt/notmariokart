#include <stdio.h>

#include "wiiuse.h" /* for wiimote_t, classic_ctrl_t, etc */

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIIUSE_WIN32
#include <unistd.h> /* for usleep */
#endif

#define MAX_WIIMOTES 1

#ifdef WIIUSE_WIN32

#include <windows.h>

void millisleep(int durationMilliseconds) { Sleep(durationMilliseconds); }

#else /* not win32 - assuming posix */

#include <unistd.h> /* for usleep */

void millisleep(int durationMilliseconds) { usleep(durationMilliseconds * 1000); }

#endif /* ifdef WIIUSE_WIN32 */

/* globals used by the simple POST implementation */
const char *g_server_url = NULL;
CURL *g_curl_handle = NULL;

struct wii_data
{
    int btn_a, btn_b, btn_one, btn_two;
    int btn_up, btn_down, btn_left, btn_right;
    int btn_minus, btn_plus, btn_home;

    int led_1, led_2, led_3, led_4;

    float pitch, yaw, roll;

    float battery_level;
    int connected;
};

void set_wii_data_event(struct wiimote_t *wm, struct wii_data *data)
{

    // Buttons
    data->btn_a = IS_PRESSED(wm, WIIMOTE_BUTTON_A);
    data->btn_b = IS_PRESSED(wm, WIIMOTE_BUTTON_B);
    data->btn_one = IS_PRESSED(wm, WIIMOTE_BUTTON_ONE);
    data->btn_two = IS_PRESSED(wm, WIIMOTE_BUTTON_TWO);
    data->btn_up = IS_PRESSED(wm, WIIMOTE_BUTTON_UP);
    data->btn_down = IS_PRESSED(wm, WIIMOTE_BUTTON_DOWN);
    data->btn_left = IS_PRESSED(wm, WIIMOTE_BUTTON_LEFT);
    data->btn_right = IS_PRESSED(wm, WIIMOTE_BUTTON_RIGHT);
    data->btn_minus = IS_PRESSED(wm, WIIMOTE_BUTTON_MINUS);
    data->btn_plus = IS_PRESSED(wm, WIIMOTE_BUTTON_PLUS);
    data->btn_home = IS_PRESSED(wm, WIIMOTE_BUTTON_HOME);

    // Orientation
    data->pitch = wm->orient.pitch;
    data->yaw = wm->orient.yaw;
    data->roll = wm->orient.roll;

    // Misc.
    data->connected = WIIMOTE_IS_CONNECTED(wm);
}

void set_wii_data_control(struct wiimote_t *wm, struct wii_data *data)
{
    // LEDs
    data->led_1 = WIIUSE_IS_LED_SET(wm, 1);
    data->led_2 = WIIUSE_IS_LED_SET(wm, 2);
    data->led_3 = WIIUSE_IS_LED_SET(wm, 3);
    data->led_4 = WIIUSE_IS_LED_SET(wm, 4);

    // Misc.
    data->battery_level = wm->battery_level;
    data->connected = WIIMOTE_IS_CONNECTED(wm);
}

struct wii_data *new_wii_data(struct wiimote_t *wm)
{
    struct wii_data *data;

    data = (struct wii_data *)malloc(sizeof(struct wii_data));
    if (data == NULL)
    {
        printf("ERROR: could not allocate memory for wii_data\n");
        exit(3);
    }

    set_wii_data_event(wm, data);
    set_wii_data_control(wm, data);

    return data;
}

const char *bool_to_str(int n) { return n ? "true" : "false"; }

char *wii_data_to_json(struct wii_data *data)
{
    if (data == NULL)
    {
        printf("ERROR: wii_data_to_json: data is NULL\n");
        return NULL;
    }

    const char *a_str = bool_to_str(data->btn_a);
    const char *b_str = bool_to_str(data->btn_b);
    const char *one_str = bool_to_str(data->btn_one);
    const char *two_str = bool_to_str(data->btn_two);
    const char *up_str = bool_to_str(data->btn_up);
    const char *down_str = bool_to_str(data->btn_down);
    const char *left_str = bool_to_str(data->btn_left);
    const char *right_str = bool_to_str(data->btn_right);
    const char *minus_str = bool_to_str(data->btn_minus);
    const char *plus_str = bool_to_str(data->btn_plus);
    const char *home_str = bool_to_str(data->btn_home);
    const char *led_1_str = bool_to_str(data->led_1);
    const char *led_2_str = bool_to_str(data->led_2);
    const char *led_3_str = bool_to_str(data->led_3);
    const char *led_4_str = bool_to_str(data->led_4);
    const char *connected_str = bool_to_str(data->connected);

    // allocate a reasonable JSON buffer
    size_t s = 4096;
    char *json = (char *)malloc(s);

    // Use snprintf with a multi-line format string to improve readability
    int n = snprintf(json, s,
                     "{\n"
                     "  \"pitch\": %.6f,\n"
                     "  \"yaw\": %.6f,\n"
                     "  \"roll\": %.6f,\n"
                     "  \"a\": %s,\n"
                     "  \"b\": %s,\n"
                     "  \"1\": %s,\n"
                     "  \"2\": %s,\n"
                     "  \"up\": %s,\n"
                     "  \"down\": %s,\n"
                     "  \"left\": %s,\n"
                     "  \"right\": %s,\n"
                     "  \"minus\": %s,\n"
                     "  \"plus\": %s,\n"
                     "  \"home\": %s,\n"
                     "  \"led_1\": %s,\n"
                     "  \"led_2\": %s,\n"
                     "  \"led_3\": %s,\n"
                     "  \"led_4\": %s,\n"
                     "  \"battery_level\": %.2f,\n"
                     "  \"connected\": %s\n"
                     "}",
                     data->pitch, data->yaw, data->roll,
                     a_str, b_str, one_str, two_str,
                     up_str, down_str, left_str, right_str,
                     minus_str, plus_str, home_str,
                     led_1_str, led_2_str, led_3_str, led_4_str,
                     data->battery_level, connected_str);

    /* check snprintf result - use size_t comparison for safety */
    if (n < 0 || (size_t)n >= s)
    {
        printf("WARNING: JSON buffer overflow or encoding error with size: %d, %zu\n", n, s);
        free(json);
        return NULL;
    }

    return json;
}

/* Helper: POST JSON to `url` using the global CURL handle */
void send_wii_json(const char *url, struct wii_data *data)
{
    if (!g_curl_handle || !url)
    {
        printf("WARNING: cannot send json data CURL or URL is NULL\n");
        return;
    }

    char *json;
    json = wii_data_to_json(data);
    if (json == NULL)
    {
        printf("WARNING: cannot send json data, JSON is NULL\n");
        return;
    }
    printf("TRACE: sending JSON: \n%s\n", json);

    // TODO: Come back to this and see if we can make it more robust.
    CURLcode res;
    curl_easy_reset(g_curl_handle);
    curl_easy_setopt(g_curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(g_curl_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(g_curl_handle, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(g_curl_handle, CURLOPT_POSTFIELDSIZE, (long)strlen(json));
    curl_easy_setopt(g_curl_handle, CURLOPT_TIMEOUT_MS, 500L);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(g_curl_handle, CURLOPT_HTTPHEADER, headers);

    res = curl_easy_perform(g_curl_handle);
    if (res != CURLE_OK)
    {
        printf("WARNING: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    curl_slist_free_all(headers);
    free(json);
}

/**
 *	@brief Callback that handles an event.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param data		Pointer to a wii_data structure.
 *
 */
void handle_event(struct wiimote_t *wm, struct wii_data *data)
{
    printf("\n\n--- EVENT [id %i] ---\n", wm->unid);

    set_wii_data_event(wm, data);

    /*
     *	Pressing minus will tell the wiimote we are no longer interested in movement.
     *	This is useful because it saves battery power.
     */
    if (data->btn_minus)
    {
        wiiuse_motion_sensing(wm, 0);
    }

    /*
     *	Pressing plus will tell the wiimote we are interested in movement.
     */
    if (data->btn_plus)
    {
        wiiuse_motion_sensing(wm, 1);
    }

    /*
     *	Pressing B will toggle the rumble
     *
     *	if B is pressed but is not held, toggle the rumble
     */
    if (data->btn_b)
    {
        wiiuse_toggle_rumble(wm);
    }

    send_wii_json(g_server_url, data);
}

/**
 *	@brief Callback that handles a read event.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param data		Pointer to the filled data block.
 *	@param len		Length in bytes of the data block.
 *
 *	This function is called automatically by the wiiuse library when
 *	the wiimote has returned the full data requested by a previous
 *	call to wiiuse_read_data().
 *
 *	You can read data on the wiimote, such as Mii data, if
 *	you know the offset address and the length.
 *
 *	The \a data pointer was specified on the call to wiiuse_read_data().
 *	At the time of this function being called, it is not safe to deallocate
 *	this buffer.
 */
void handle_read(struct wiimote_t *wm, byte *data, unsigned short len)
{
    int i = 0;

    printf("\n\n--- DATA READ [wiimote id %i] ---\n", wm->unid);
    printf("finished read of size %i\n", len);
    for (; i < len; ++i)
    {
        if (!(i % 16))
        {
            printf("\n");
        }
        printf("%x ", data[i]);
    }
    printf("\n\n");
}

/**
 *	@brief Callback that handles a controller status event.
 *
 *	This occurs when either the controller status changed
 *	or the controller status was requested explicitly by
 *	wiiuse_status().
 *
 *	One reason the status can change is if the nunchuk was
 *	inserted or removed from the expansion port.
 */
void handle_ctrl_status(struct wiimote_t *wm, struct wii_data *data)
{
    set_wii_data_control(wm, data);
    send_wii_json(g_server_url, data);
}

/**
 *	@brief Callback that handles a disconnection event.
 *
 *	@param wm				Pointer to a wiimote_t structure.
 *
 *	This can happen if the POWER button is pressed, or
 *	if the connection is interrupted.
 */
void handle_disconnect(wiimote *wm, struct wii_data *data)
{
    printf("INFO: wii remote [%d] disconnected\n", wm->unid);
    data->connected = 0;
    send_wii_json(g_server_url, data);
}

short any_wiimote_connected(wiimote **wm, int wiimotes)
{
    int i;
    if (!wm)
    {
        return 0;
    }

    for (i = 0; i < wiimotes; i++)
    {
        if (wm[i] && WIIMOTE_IS_CONNECTED(wm[i]))
        {
            return 1;
        }
    }

    return 0;
}

int connect_to_wiimotes(wiimote **wm, int num_wiimotes, int num_wiimotes_found)
{
    const int MAX_ATTEMPTS_CONNECT = 3;
    int connected;

    printf("DEBUG: attempting to connect to %d wii remotes\n", num_wiimotes);

    for (int i = 0; i < MAX_ATTEMPTS_CONNECT; i++)
    {
        connected = wiiuse_connect(wm, num_wiimotes);
        if (connected)
        {
            printf("INFO: connected to %i/%i wii remotes.\n", connected, num_wiimotes_found);
            break;
        }

        printf("WARNING: failed to connect to any wiimote attempt %d/%d\n", i + 1, MAX_ATTEMPTS_CONNECT);
    }

    return connected;
}

int find_wiimotes(wiimote **wm, int num_wiimotes)
{
    const int TIMEOUT_IN_SECONDS = 1;
    const int MAX_ATTEMPTS_FIND = 100;

    printf("DEBUG: find_wiimotes: attempting to find %d wii remotes\n", num_wiimotes);

    int found;
    for (int i = 0; i < MAX_ATTEMPTS_FIND; i++)
    {
        found = wiiuse_find(wm, num_wiimotes, TIMEOUT_IN_SECONDS);
        if (found)
        {
            break;
        }
        printf("DEBUG: find_wiimotes: no wii remotes found attempt %d/%d\n", i + 1, MAX_ATTEMPTS_FIND);
    }

    return found;
}

int find_and_connect_wiimotes(wiimote **wiimotes, int num_wiimotes)
{
    // Find the wii remotes.
    int found;
    found = find_wiimotes(wiimotes, num_wiimotes);
    if (found <= 0)
    {
        printf("DEBUG: could not find any wii remotes, try again!\n");
        return found;
    }

    // Connect to the wii remotes.
    return connect_to_wiimotes(wiimotes, num_wiimotes, found);
}

// cleanup cleans up the wiimotes and the curl handle.
void cleanup(wiimote **wiimotes, int num_wiimotes, CURL *curl_handle)
{
    wiiuse_cleanup(wiimotes, num_wiimotes);

    if (curl_handle)
    {
        curl_easy_cleanup(curl_handle);
    }
    curl_global_cleanup();
}

void connect_to_server(const char *address)
{
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
    {
        printf("WARNING: curl_global_init failed\n");
        return;
    }

    g_server_url = address;

    g_curl_handle = curl_easy_init();
    if (!g_curl_handle)
    {
        printf("WARNING: curl_easy_init failed; HTTP POSTs will be disabled\n");
    }

    return;
}

void loop(wiimote **wiimotes, int max_wiimotes, struct wii_data *data)
{
    /*
     *	This is the main loop
     *
     *	wiiuse_poll() needs to be called with the wiimote array
     *	and the number of wiimote structures in that array
     *	(it doesn't matter if some of those wiimotes are not used
     *	or are not connected).
     *
     *	This function will set the event flag for each wiimote
     *	when the wiimote has things to report.
     */
    while (any_wiimote_connected(wiimotes, max_wiimotes))
    {
        if (wiiuse_poll(wiimotes, max_wiimotes))
        {
            /*
             *	This happens if something happened on any wiimote.
             *	So go through each one and check if anything happened.
             */
            int i = 0;
            for (; i < max_wiimotes; ++i)
            {
                switch (wiimotes[i]->event)
                {
                case WIIUSE_EVENT:
                    /* a generic event occurred */
                    handle_event(wiimotes[i], data);
                    break;

                case WIIUSE_STATUS:
                    /* a status event occurred */
                    handle_ctrl_status(wiimotes[i], data);
                    break;

                case WIIUSE_DISCONNECT:
                    printf("Wiimote %d disconnected normally.\n", wiimotes[i]->unid);
                    handle_disconnect(wiimotes[i], data);
                    break;

                case WIIUSE_UNEXPECTED_DISCONNECT:
                    /* the wiimote disconnected */
                    handle_disconnect(wiimotes[i], data);
                    break;

                case WIIUSE_READ_DATA:
                    /*
                     *	Data we requested to read was returned.
                     *	Take a look at wiimotes[i]->read_req
                     *	for the data.
                     */
                    printf("INFO: data read event occurred, but we are not processing it\n");
                    break;

                default:
                    printf("INFO: unknown event of type %d occurred\n", wiimotes[i]->event);
                    break;
                }
            }
        }
    }
}

/**
 *	@brief main()
 *
 *	Connect to up to two wiimotes and print any events
 *	that occur on either device.
 */
int main(int argc, char **argv)
{
    /* server base (no trailing /). Optional override via argv[1]. */
    const char *server_address = "http://localhost:8080/wii";
    if (argc > 1 && argv[1] && argv[1][0] != '\0')
    {
        server_address = argv[1];
    }

    // Connect to the web server where we will be sending the Wii remote data
    // to via HTTP POST with JSON payloads.
    connect_to_server(server_address);

    // Initialize an array of wiimote objects.
    wiimote **wiimotes;
    wiimotes = wiiuse_init(MAX_WIIMOTES);

    int connected = find_and_connect_wiimotes(wiimotes, MAX_WIIMOTES);
    if (connected <= 0)
    {
        printf("ERROR: could not connect to any wii remotes, try again!\n");
        return -1;
    }

    /*
     *	Now set the LEDs and rumble for a second so it's easy
     *	to tell which wiimotes are connected (just like the wii does).
     */
    wiiuse_set_leds(wiimotes[0], WIIMOTE_LED_1);
    wiiuse_rumble(wiimotes[0], 1);
    millisleep(200);
    wiiuse_rumble(wiimotes[0], 0);
    millisleep(200);
    wiiuse_rumble(wiimotes[0], 1);
    millisleep(200);
    wiiuse_rumble(wiimotes[0], 0);

    // Generates a WIIUSE_STATUS event.
    wiiuse_status(wiimotes[0]);

    printf("\nControls:\n");
    printf("\tB toggles rumble.\n");
    printf("\t+ to start Wiimote accelerometer reporting, - to stop\n");
    printf("\n\n");

    // TODO: This should be an array of wii_data, one per wiimote.
    // This is fine for now since we are only using one wiimote.
    struct wii_data *data = new_wii_data(wiimotes[0]);

    // Main loop that polls for events and handles them.
    loop(wiimotes, MAX_WIIMOTES, data);

    // TODO: Add automatic reconnection logic.
    // For now we just exit when all wiimotes are disconnected.

    cleanup(wiimotes, MAX_WIIMOTES, g_curl_handle);

    return 0;
}
