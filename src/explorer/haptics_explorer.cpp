#include "haptics_explorer.h"
// For semaphores (asyncQueues)
static char        gSTRT[]    = "started";
static char        gERROR[]   = "error";
static char        gDONE[]    = "done";

HapticsExplorer::HapticsExplorer(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade) :
        Gtk::Window(cobject),
        m_ref_glade(refGlade),
        m_btn_quit(nullptr),
        m_btn_send(nullptr) {
    // Get the Glade-instantiated Button, and connect a signal handler:
    m_ref_glade->get_widget("quit_button", m_btn_quit);
    if (m_btn_quit) {
        m_btn_quit->signal_clicked().connect(sigc::mem_fun(*this, &HapticsExplorer::on_button_quit));
    }
    m_ref_glade->get_widget("send_button", m_btn_send);
    if (m_btn_send) {
        m_btn_send->signal_clicked().connect(sigc::mem_fun(*this, &HapticsExplorer::on_button_send));
    }
}

// Call this after calling Gtk::Builder::get_widget_derived().
void HapticsExplorer::init(void) {
    // Rest of the initialization

    // Get Adjustments handles
    CONNECT_SCALE(XLocation);
    //CONNECT_SCALE(YHeight);
    CONNECT_SCALE(ZDepth);

    // Switch 1
    m_ref_glade->get_widget("switch_SendContinuously", m_switch_SendContinuously);
    m_switch_SendContinuously->set_active(true);
    SendContinuously = true;
    cout << "SendContinuously is set for " << SendContinuously << endl;
    m_switch_SendContinuously->property_active().signal_changed().connect(sigc::mem_fun(*this, &HapticsExplorer::on_SendContinuously_active_changed));

    // Switch 2
    m_ref_glade->get_widget("switch_SendImmediately", m_switch_SendImmediately);
    m_switch_SendImmediately->set_active(false);
    SendImmediately = false;
    cout << "SendImmediately is set for " << SendImmediately << endl;
    m_switch_SendImmediately->property_active().signal_changed().connect(sigc::mem_fun(*this, &HapticsExplorer::on_SendImmediately_active_changed));

    // Radio Buttons
    m_ref_glade->get_widget("rd0", m_rd_ObjectType_0);
    m_ref_glade->get_widget("rd1", m_rd_ObjectType_1);
    m_ref_glade->get_widget("rd2", m_rd_ObjectType_2);
    m_ref_glade->get_widget("rd3", m_rd_ObjectType_3);
    m_rd_ObjectType_0->set_active();
    ObjectType = ObjType_t::Object;
    cout << "ObjectType is set to " << ObjectType << endl;
    m_rd_ObjectType_0->signal_toggled().connect(sigc::mem_fun(*this, &HapticsExplorer::on_ObjectType_toggled));
    m_rd_ObjectType_1->signal_toggled().connect(sigc::mem_fun(*this, &HapticsExplorer::on_ObjectType_toggled));
    m_rd_ObjectType_2->signal_toggled().connect(sigc::mem_fun(*this, &HapticsExplorer::on_ObjectType_toggled));
    m_rd_ObjectType_3->signal_toggled().connect(sigc::mem_fun(*this, &HapticsExplorer::on_ObjectType_toggled));



    initBLE();
}

HapticsExplorer::~HapticsExplorer() {
	// Close BLE connection
	closeBLE();
}

void HapticsExplorer::initBLE() {
}

void HapticsExplorer::closeBLE() {
}

void HapticsExplorer::sendData() {
	g_mutex_lock(&snd_mutex); // this mutex guard is to ensure that there is only one send op at any given time
    g_async_queue_pop(blereqsem); // Wait for sender to be ready

    // Final 12-byte data structure including CRC
    bzero((char *) &data, sizeof(objectData_t));
    data.sop      = '!';
    data.pkt_type = 'O';
    data.objs[0].obj   = ObjectType; //ObjType_t::Stairs; // Object; // ...
	data.objs[0].x     = XLocation;
	data.objs[0].depth = ZDepth;
    data.objs[1].obj   = ObjType_t::Object;  // Second object.
    data.objs[1].x     = 255;
    data.objs[1].depth = 255;
    for (int i = 0; i < (sizeof(objectData_t) - 1); i++)
        data.cksum += (unsigned char) ((char *) &data)[i];
    data.cksum = ~data.cksum;

	g_print("Sending data: x:%3d, depth:%3d, cksum=%3d\n", data.objs[0].x, data.objs[0].depth, data.cksum);
    g_async_queue_push(blerspsem, (gpointer) gDONE); // Signal data availability to sender
	g_mutex_unlock(&snd_mutex);
}

void HapticsExplorer::doSendData() {
    sendData();
}

void HapticsExplorer::setSemaphores(GAsyncQueue* reqsem, GAsyncQueue* rspsem, GMutex& mutex) {
	blereqsem = reqsem;
	blerspsem = rspsem;
	obj_mutex= mutex;
}

// Data validation: this app doesn't need specific checks or modification.
// Kept for future upgrade possibility. Sample below
void HapticsExplorer::check_XLocation_valid_value() {
    // the XLocation must be between 0 and 255
	// This check is not actually required. Just an example.
    if (XLocation < 0) {
    	XLocation = 0;
    } else if (XLocation > 255) {
    	XLocation = 255;
    }
    m_adj_XLocation->set_value(XLocation);
}

//void HapticsExplorer::check_YHeight_valid_value() {
//
//}

void HapticsExplorer::check_ZDepth_valid_value() {

}


