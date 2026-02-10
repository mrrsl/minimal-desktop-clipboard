#pragma once
/*
Remember to compile with $(pkg-config --libs --cflags x11)
*/

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <pthread.h>

#include <string>
#include <cstring>
#include <vector>

#include <iostream>

// Defines the maximum number of atoms to read for when requesting the list of targets
#define CLIPLIB_ATOM_LIMIT 512

namespace ClipLibX11 {
// Connection to the X server
Display* PrimaryDisplay = XOpenDisplay(NULL);
// X server connection for SetClipboard functions
Display* ClipDisplay = nullptr;
// Root Window
Window RootW = RootWindow(PrimaryDisplay, DefaultScreen(PrimaryDisplay));
// Atom for the clipboard selection
Atom ClipAtom = XInternAtom(PrimaryDisplay, "CLIPBOARD", False);
// Atom for retrieving conversion targets
Atom TargetAtom = XInternAtom(PrimaryDisplay, "TARGETS", False);
// Atom for retrieving a UTF8 text conversion
Atom Utf8Atom = XInternAtom(PrimaryDisplay, "UTF8_STRING", False);
// Atom for the TEXT target
Atom TextAtom = XInternAtom(PrimaryDisplay, "TEXT", False);
// Atom for the TIMESTAMP target
Atom TimeStampAtom = XInternAtom(PrimaryDisplay, "TIMESTAMP", False);
// Atom for the INCR conversion type
Atom IncrAtom = XInternAtom(PrimaryDisplay, "INCR", False);
// Temporary window to hold the result of our conversion request
Window HoldingW = XCreateSimpleWindow(PrimaryDisplay, RootW, -10, -10, 1, 1, 0, 0, 0);
// Property on which the result of the conversion is stored
Atom HoldingAtom = XInternAtom(PrimaryDisplay, "CLIP_LIB", False);
// Temporary window to respond to selection requests
Window ClipWindow = None;
// Length (including terminating null) of the last string copied to clipboard.
unsigned TextLength = 0;
// Arbritrary size after which library will use the INCR mechanism to transfer clipboard data to requestors
unsigned long ClipboardIncrThreshold = 1 << 20;
// Internal pointer for our data since std::string::data does not transfer ownership
char* TextData = nullptr;
// Internal pointer for available targets
Atom* TargetData = nullptr;
// Lenght of TargetData
unsigned TargetDataLength = 0;
// Thread handle for our selection request event loop
pthread_t ClipThread;
// Lock for our own clipboard data
pthread_mutex_t DataMutex = PTHREAD_MUTEX_INITIALIZER; 
// Xlib concurrency support
Status ThreadInit = XInitThreads();

bool HasTarget(Atom searchTarget, std::vector<Atom> &targets);

// Send a Conversion request to obtain a list of targets. Return false if a connection to the XServer was not established.
bool Internal_RequestClipboardTargets();
// Returns a vector of all available clipboard targets after Internal_RequestClipboardTargets is called.
std::vector<Atom> Internal_RetrieveClipboardTargets();

// Populates a vector with existing clipboard targets, return true if vector has been successfully populated.
bool GetClipboardTargets(std::vector<Atom>& targets);

// Request conversion of the clipboard slection as a UTF8 String
bool Internal_RequestSelectionUtf8();

// Implementation of the X11 INCR mechanism
char* Internal_RetrieveIncrUtf8 ();

// Returns a char array for a UTF 8 string after a call to Internal_RequestSelectionUtf8. If string data is too large, it will instead return a nullptr.
char* Internal_RetrieveSelectionUtf8();

// Returns a string matching the text on the clipboard selection
std::string GetSelectionAsUtf8();

// Relinquishes ownership of the CLIPBOARD selection to the X server if selection is owned by ClipWindow.
void RelinquishClipboard();

// Sets data to be sent back on a request for the CLIPBOARD selection
bool SetClipboardText(char* str);

// Procedure for responding to requests for data set by SetClipboard
void* SetClipboardTextProc(void* data);

// Sends Clipboard Text data to a requestor
void Internal_SendClipboardText(XSelectionRequestEvent event, Display*);

// Sends an array of available format targets to a requestor
void Internal_SendTargetData(XSelectionRequestEvent event, Display*);

// Thread cleanup for SetClipboardTextProc
void CleanupSetClipboard(void*);

// Constructs an event to send as a reply to a SelectionRequest
XSelectionEvent ConstructSelectionEvent(XSelectionRequestEvent event);
} // namespace ClipLibX11


// Returns true if the list of targets contains the specified Atom.
bool ClipLibX11::HasTarget(Atom searchTarget, std::vector<Atom> &targets) {
    
    for (std::size_t a = 0; a < targets.size(); a++) {
        if (targets[a] == searchTarget)
            return true;
    }
    return false;
}

// Send a Conversion request to obtain a list of targets. Return false if a connection to the XServer was not established.
bool ClipLibX11::Internal_RequestClipboardTargets() {
    
    if (PrimaryDisplay == None)
        return false;

    XConvertSelection(PrimaryDisplay, ClipAtom, TargetAtom, HoldingAtom, HoldingW, CurrentTime);
    return true;
}
// Returns a vector of all available clipboard targets after Internal_RequestClipboardTargets is called.
std::vector<Atom> ClipLibX11::Internal_RetrieveClipboardTargets() {
    
    Atom dummyAtom, actualType;
    int dummyInt;
    unsigned long nItems, dummyUl;
    unsigned char* data;

    XGetWindowProperty(PrimaryDisplay, HoldingW, HoldingAtom, 0, CLIPLIB_ATOM_LIMIT * sizeof(Atom), True, XA_ATOM, &actualType, &dummyInt, &nItems, &dummyUl, &data);
    Atom* dataTargets = (Atom*)data;
    std::vector<Atom> targets(nItems, None);

    for (unsigned a = 0; a <nItems; a++) {
        targets[a] = dataTargets[a];
    }
    XFree(data);
    return targets;
}

// Populates a vector with existing clipboard targets, return true if vector has been successfully populated.
bool ClipLibX11::GetClipboardTargets(std::vector<Atom>& targets) {
    
    if (!Internal_RequestClipboardTargets())
        return false;

    XEvent event;

    while(1) {
        XNextEvent(PrimaryDisplay, &event);
        if (event.type == SelectionNotify) {
            XSelectionEvent &sEvent = event.xselection;
            if (sEvent.property == None) return false;
            else {
                targets = Internal_RetrieveClipboardTargets();
                return true;
            }
        }
    
    }
}

// Request conversion of the clipboard slection as a UTF8 String
bool ClipLibX11::Internal_RequestSelectionUtf8() {
    
    if (PrimaryDisplay == None)
        return false;

    XConvertSelection(PrimaryDisplay, ClipAtom, Utf8Atom, HoldingAtom, HoldingW, CurrentTime);
    return true;
}

// Implementation of the X11 INCR mechanism
char* ClipLibX11::Internal_RetrieveIncrUtf8 () {
    
    Atom dummyAtom, actualType;
    int dummyInt;
    unsigned long dummyUl, size;
    unsigned char* data = nullptr;

    XGetWindowProperty(PrimaryDisplay, HoldingW, HoldingAtom, 0, 0, True, AnyPropertyType, &actualType, &dummyInt, &dummyUl, &dummyUl, &data);
    char* str = new char[*(int*)data];
    unsigned int index = 0;

    XFree(data);

    // Steps for the INCR chunking mechanism desribed: https://x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html#incr_properties
    XEvent event;

    while (1) {
        XNextEvent(PrimaryDisplay, &event);
        if (event.type == SelectionNotify || PropertyNotify) {           
            XGetWindowProperty(PrimaryDisplay, HoldingW, HoldingAtom, 0, 0, False, AnyPropertyType, &actualType, &dummyInt, &dummyUl, &size, &data);
            XFree(data);

            if (size == 0)
                return str;

            XGetWindowProperty(PrimaryDisplay, HoldingW, HoldingAtom, 0, size, True, AnyPropertyType, &actualType, &dummyInt, &dummyUl, &dummyUl, &data);
            std::memcpy(str + index, data, size);
            XFree(data);

            index += size;

        } 
    
    }
}

// Returns a char array for a UTF 8 string after a call to Internal_RequestSelectionUtf8. If string data is too large, it will instead return a nullptr.
char* ClipLibX11::Internal_RetrieveSelectionUtf8() {
    
    Atom actualType;
    int dummyInt;
    unsigned long dummyUl, size;
    unsigned char* data = nullptr;

    XGetWindowProperty(PrimaryDisplay, HoldingW, HoldingAtom, 0, 0, False, AnyPropertyType, &actualType, &dummyInt, &dummyUl, &size, &data);
    
    if (actualType == IncrAtom) {
        char *text = Internal_RetrieveIncrUtf8();
        XFree(data);
        return text;
    }
    XFree(data);
    XGetWindowProperty(PrimaryDisplay, HoldingW, HoldingAtom, 0, size, False, AnyPropertyType, &actualType, &dummyInt, &dummyUl, &dummyUl, &data);
    
    char* text = new char[size + 1];
    
    std::memcpy(text, (char*)data, size);
    text[size] = '\0';
    XFree(data);
    XDeleteProperty(PrimaryDisplay, HoldingW, HoldingAtom);
    
    return text;
}

// Returns a string matching the text on the clipboard selection
std::string ClipLibX11::GetSelectionAsUtf8() {
    
    std::vector<Atom> targets;
    
    if (GetClipboardTargets(targets) && HasTarget(Utf8Atom, targets)) {
        
        Internal_RequestSelectionUtf8();
        
        while(1) {
            
            XEvent event;
            XNextEvent(PrimaryDisplay, &event);
            
            if (event.type == SelectionNotify) {
                
                XSelectionEvent &sEvent = event.xselection;
                
                if (sEvent.property != None) {
                    
                    char* rawResult = Internal_RetrieveSelectionUtf8();
                    std::string result(rawResult);
                    
                    delete[] rawResult;
                    return result;

                } else {
                    return std::string();
                }
            }
                
                
        }
    }

    else return std::string();
}

// Relinquishes ownership of the CLIPBOARD selection to the X server if selection is owned by ClipWindow.
void ClipLibX11::RelinquishClipboard() {
    if (XGetSelectionOwner(PrimaryDisplay, ClipAtom) == ClipWindow) {
        pthread_cancel(ClipThread);
    }
}
// Cleans up data allocated by the SetClipboardText call
void ClipLibX11::CleanupSetClipboard(void* data) {
    // This can be called if SetClipboardText fails to properly allocate its required resources
    if (ClipWindow != None) {
        XDestroyWindow(ClipLibX11::ClipDisplay, ClipLibX11::ClipWindow);
        ClipWindow = None;
    }
    
    TextLength = 0;
    
    if (TextData != nullptr) {
        delete[] TextData;
        TextData = nullptr;
    }
    
    if (TargetData != nullptr) {
        delete[] TargetData;
        TargetData = nullptr;
    }
    
    TargetDataLength = 0;

    if (ClipDisplay != nullptr) {
        XSetSelectionOwner(ClipDisplay, ClipAtom, None, CurrentTime);
        XCloseDisplay(ClipDisplay);
        ClipDisplay = nullptr;
    }
}

void* ClipLibX11::SetClipboardTextProc(void* data) {
    // TODO: Implement INCR chunking
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_cleanup_push(CleanupSetClipboard, data);

    XEvent event;
    bool cont = true;
    while(cont) {
        
        XNextEvent(ClipDisplay, &event);
        
        if (event.type == SelectionRequest) {
            
            XSelectionRequestEvent& reqEvent = event.xselectionrequest;
            
            if (reqEvent.owner == ClipWindow && reqEvent.selection == ClipAtom) {
                
                if (reqEvent.target == Utf8Atom || reqEvent.target == TextAtom) {
                    Internal_SendClipboardText(reqEvent, ClipDisplay);
                } else if (reqEvent.target == TargetAtom) {
                    Internal_SendTargetData(reqEvent, ClipDisplay);
                }
                        
            }
        } else if (event.type == SelectionClear) {
            cont = false;
        }
    }

    pthread_cleanup_pop(1);
    
    return nullptr;
}

// Sets data to be sent back on a request for the CLIPBOARD selection. Note that neither library or this function will take ownership of the given string.
bool ClipLibX11::SetClipboardText(char* str) {
    
    pthread_mutex_lock(&DataMutex);
    
    if (ClipWindow == None) {
        
        ClipDisplay = XOpenDisplay(NULL);
        
        try {
            TextLength = strlen(str) + 1;
            TextData = new char[TextLength];
            // TODO: Remove dynamic allocations for target data
            TargetData = new Atom[3];
            TargetData[0] = Utf8Atom;
            TargetData[1] = TimeStampAtom;
            TargetData[2] = TextAtom;
            TargetDataLength = 3;
            
            std::memcpy(TextData, str, TextLength);
            ClipWindow = XCreateSimpleWindow(ClipDisplay, RootW, -9, -9, 1, 1, 0, 0, 0);
        
        } catch (std::exception e) {
            pthread_mutex_unlock(&DataMutex);
            CleanupSetClipboard(str);
            return false;
        }
        
        pthread_mutex_unlock(&DataMutex);
        pthread_create(&ClipThread, NULL, SetClipboardTextProc, (void*)TextData);

        XSetSelectionOwner(ClipDisplay, ClipAtom, ClipWindow, CurrentTime);
        
        if (XGetSelectionOwner(ClipDisplay, ClipAtom) != ClipWindow) {
            pthread_cancel(ClipThread);
            return false;
        }
        return true;
    } else {
        pthread_mutex_unlock(&DataMutex);
        CleanupSetClipboard(str);
        return SetClipboardText(str);
    }
}

XSelectionEvent ClipLibX11::ConstructSelectionEvent(XSelectionRequestEvent event) {
    
    XSelectionEvent payload;
    payload.type = SelectionNotify;
    payload.requestor = event.requestor;
    payload.time = CurrentTime;
    payload.selection = ClipAtom;
    
    if (event.property == None)
        payload.property = event.target;
    else
        payload.property = event.property;
    
    return payload;
}

void ClipLibX11::Internal_SendClipboardText(XSelectionRequestEvent event, Display* display) {
    XSelectionEvent payload = ConstructSelectionEvent(event);
    payload.target = Utf8Atom;

    XChangeProperty(display, event.requestor, event.property, XA_STRING, 8, PropModeReplace, (unsigned char*)TextData, (int)TextLength);
    XSendEvent(display, event.requestor, True, NoEventMask, (XEvent*)&payload);
}

void ClipLibX11::Internal_SendTargetData(XSelectionRequestEvent event, Display* display) {
    XSelectionEvent payload = ConstructSelectionEvent(event);
    payload.target = TargetAtom;

    XChangeProperty(display, event.requestor, event.property, XA_ATOM, 32, PropModeReplace, (unsigned char*)TargetData, (int)TargetDataLength);
    XSendEvent(display, event.requestor, True, NoEventMask, (XEvent*)&payload);
}
