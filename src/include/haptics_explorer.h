
#ifndef _HAPTICS_EXPLORER_H_
#define _HAPTICS_EXPLORER_H_

#include <dvia_common.h>
#include <gtkmm.h>
#include <iostream>
#include "hci.hpp"           // HCI - BLE Scanner
#include "gatt.hpp"          // Gatt - BLE connection and transfer

using namespace std;


#define DECLARE_SCALE(XX, TYPE) \
  Gtk::Scale*                    m_s_##XX; \
  Glib::RefPtr<Gtk::Adjustment>  m_adj_##XX; \
  TYPE                           XX

#define CONNECT_SCALE(XX) \
  m_ref_glade->get_widget("s_"#XX, m_s_##XX); \
  m_adj_##XX = m_s_##XX->get_adjustment(); \
  XX = m_adj_##XX->get_value(); \
  /*cout.precision(2); cout << "Initial value of " #XX ":" << XX << endl;*/ \
  m_adj_##XX->signal_value_changed().connect(sigc::mem_fun(*this, &HapticsExplorer::on_##XX##_value_changed))



// Normally we would call sendData() only when scale changed. In that case
// it would be called from this(these) function(s). However,
// for this app, we want to periodically send data to the band,
// so, we don't call sendData here
#define ON_SCALE_CHANGE(XX) \
void on_##XX##_value_changed() { \
    XX = m_adj_##XX->get_value(); \
    check_##XX##_valid_value(); \
    /*cout.precision(2); cout << #XX " changed: " << XX << endl;*/ \
}


//typedef void (*cb_send_data2band) (objectData_t_t& data);

class HapticsExplorer: public Gtk::Window {
public:
    HapticsExplorer(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
    void init();
    virtual ~HapticsExplorer();

    void sendData();
    void doSendData();
    //void set_cb_send_data2band(cb_send_data2band cb);
    void setSemaphores(GAsyncQueue* reqsem, GAsyncQueue* rspsem, GMutex& mutex);
    objectData_t& extractData() {
    	return data;
    }

    // Public parameters
    bool                           SendContinuously; // If true, send data continuously to the band; otherwise, only on change
    bool                           SendImmediately;  // If true, send data as soon as data changes; else, defer to SendContinuously setting
    // Effect of above: When both the switches are at off position, data will be sent only on the press of 'Send' button.

protected:
    //Signal handlers:
    void on_button_quit() {
        hide(); //hide() will cause Gtk::Application::run() to end.
    }
    void on_button_send() {
        // Pressing this button will turn off the SendContinuously switch
        if (SendContinuously)
            m_switch_SendContinuously->set_active(false);
        sendData();
    }
    void initBLE();
    void closeBLE();

    // Check valid values
    void check_XLocation_valid_value();
    void check_YHeight_valid_value();
    void check_ZDepth_valid_value();

    ON_SCALE_CHANGE(XLocation)
    ON_SCALE_CHANGE(YHeight)
    ON_SCALE_CHANGE(ZDepth)

    void on_SendContinuously_active_changed() {
    	SendContinuously = m_switch_SendContinuously->get_active();
        cout << "SendContinuously changed: " << SendContinuously << endl;
    }
    void on_SendImmediately_active_changed() {
    	SendImmediately = m_switch_SendImmediately->get_active();
        cout << "SendImmediately changed: " << SendImmediately << endl;
    }

    // Local variables
    Glib::RefPtr<Gtk::Builder>     m_ref_glade;
    Gtk::Button*                   m_btn_quit;
    Gtk::Button*                   m_btn_send;
    DECLARE_SCALE(XLocation, int);
    DECLARE_SCALE(YHeight, int);
    DECLARE_SCALE(ZDepth, int);


    // Switches
    Gtk::Switch*                   m_switch_SendContinuously;
    Gtk::Switch*                   m_switch_SendImmediately;
    objectData_t                   data;
    GAsyncQueue* blerspsem; // used between proc and sender
    GAsyncQueue* blereqsem; // used between proc and sender
    GMutex obj_mutex;       // a common/global obj_mutex shared with calling application. Not used right now.
    GMutex snd_mutex;       // for exclusive access to sendData function
};

#endif //_HAPTICS_EXPLORER_H_
