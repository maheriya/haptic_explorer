#include <iostream>
#include "dvia_common.h"
#include "dvia_sender.hpp"
#include "haptics_explorer.h"

// Period of the timer for sending data to the band
#define TIMER_PERIOD 300
using namespace std;

static HapticsExplorer* explorer = nullptr;
static GMutex obj_mutex;     // used to make sure object data is accessed properly


gboolean cbSendData(gpointer data);


static dviaSender sender;
pthread_t sender_thread;
static GAsyncQueue* blerspsem; // used between proc and sender
static GAsyncQueue* blereqsem; // used between proc and sender
// For semaphores (asyncQueues)
static char        gSTRT[]    = "started";
static char        gERROR[]   = "error";
static char        gDONE[]    = "done";

static void* sender_thread_func(void *tid) {
    double tperf;
    g_async_queue_push(blereqsem, (gpointer) gSTRT); // Send request to explorer for data
    while (1) {
        //tperf = getTickCount(); // outside the wait. Gets us full time on average
        g_async_queue_pop(blerspsem); // Wait for data from proc
        sender.send(explorer->extractData());
        g_async_queue_push(blereqsem, (gpointer) gSTRT); // Send request to proc for data
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv) {

    blereqsem = g_async_queue_new(); // sender will ask for more data when ready (push); explorer waits for it (pop)
    blerspsem = g_async_queue_new(); // explorer will send data (push); sender waits for data (pop)

    sender.init();
    int rtid[4] = { 10, 11, 12, 13 };
    if (pthread_create(&sender_thread, NULL, sender_thread_func, rtid + 2)) {
        printf("ERROR: Couldn't create sender_thread\n");
        exit(-1);
    }

    auto app = Gtk::Application::create(argc, argv, "dvia.haptics.explorer");

    // Load the Glade file and instantiate its widgets:
    auto refBuilder = Gtk::Builder::create();
    try {
        refBuilder->add_from_file("data/explorer.glade");
    } catch (const Glib::FileError& ex) {
        std::cerr << "FileError: " << ex.what() << std::endl;
        return 1;
    } catch (const Glib::MarkupError& ex) {
        std::cerr << "MarkupError: " << ex.what() << std::endl;
        return 1;
    } catch (const Gtk::BuilderError& ex) {
        std::cerr << "BuilderError: " << ex.what() << std::endl;
        return 1;
    }

    // Get the GtkBuilder-instantiated window:
    refBuilder->get_widget_derived("HapticsExplorer", explorer);
    explorer->init();
    explorer->setSemaphores(blereqsem, blerspsem, obj_mutex);
    //explorer->set_cb_send_data2band(doSendData);

    int cbdata = 20;
    int status = g_timeout_add (TIMER_PERIOD, cbSendData, &cbdata);
    if (explorer)
        app->run(*explorer);

    pthread_cancel(sender_thread);
    sender.stop();
    cout << "Done!" << endl;

    // End of application
    delete explorer;

    return 0;
}


gboolean cbSendData(gpointer data) {
#if END_AFTER_FIXED_NUM_OF_ITERATIONS
  static guint16 i=0;
  int mydata = *((int*) data);
  g_print("Sending data: Iter=%" G_GUINT16_FORMAT "\n",i++);
  if (i >= mydata) {
    g_print("Done!\n"); //, data);
    return FALSE;
  }
#endif
  if (explorer->SendContinuously)
      explorer->sendData();
  return TRUE;
}
