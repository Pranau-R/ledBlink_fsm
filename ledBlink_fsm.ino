/*
Module: ledBlink_fsm.ino

Function:
        It is a simple example program for FSM using LED.

Copyright notice:
        This file copyright (C) 2022 by
        MCCI Corporation
        3520 Krums Corners Road
        Ithaca, NY 14850
        An unpublished work. All rights reserved.
        This file is proprietary information, and may not be disclosed or
        copied without the prior permission of MCCI Corporation.

Author:
        Pranau R, MCCI Corporation   June 2022
*/

#include <Catena.h>
#include <Catena_FSM.h>
#include <Catena_Led.h>
#include <Catena_CommandStream.h>
#include <Catena_CommandStream_vmicro_fixup.h>

using namespace McciCatena;

class BlinkLED
    {
public:
    BlinkLED(Catena &myCatena, StatusLed &blink)
        : m_Catena(myCatena),
          m_Led(blink)
        {}

    enum State
        {
        stNoChange = 0, // this name must be present: indicates "no change of state"
        stInitial,      // this name must be presnt: it's the starting state.
        stON,           // the LED is HIGH.
        stOFF,          // the LED is LOW.
        stFinal,        // this name must be present, it's the terminal state.
        };

    // the begin method initializes the fsm
    void begin()
        {
        if (! this->m_fRunning)
            this->m_fsm.init(*this, &BlinkLED::fsmDispatch);
        else
            this->m_Catena.SafePrintf("already running!\n");
        }

    // the end method shuts it down
    void end()
        {
        if (this->checkRunning())
            {
            this->m_evShutdown = true;
            while (this->m_fRunning)
            this->m_fsm.eval();
            }
        }

    // ON event
    void evON()
        {
        if (this->checkRunning())
            {
            this->m_evON = true;
            this->m_fsm.eval();
            }
        }

    // OFF event
    void evOFF()
        {
        if (this->checkRunning())
            {
            this->m_evOFF = true;
            this->m_fsm.eval();
            }
        }

    // stop event
    void evStop()
        {
        if (this->checkRunning())
            {
            this->m_evShutdown = true;
            this->m_fsm.eval();
            }
        }

private:
    // the FSM instance
    cFSM <BlinkLED, State> m_fsm;

    // verify that FSM is running, and print a message if not.
    bool checkRunning() const
        {
        if (this->m_fRunning)
            return true;
        else
            {
            this->m_Catena.SafePrintf("not running!\n");
            return false;
            }
        }

    void setBlink(bool fState)
        {
        if (fState)
            {
            m_Catena.SafePrintf("**LED is LOW**\n");
            }
        else
            {
            m_Catena.SafePrintf("**LED is HIGH**\n");
            }
        }

    State fsmDispatch(State currentState, bool fBlink)
        {
        State newState = State::stNoChange;

        switch (currentState)
            {
            case State::stInitial:
                if (fBlink)
                    {
                    // Initially the LED is LOW.
                    }
                this->m_fRunning = true;
                newState = State::stOFF;
                break;

            case State::stOFF:
                if (fBlink)
                    {
                    this->setBlink(true);
                    }
                if (this->m_evShutdown)
                    {
                    this->m_evShutdown = false;
                    m_Led.Set(LedPattern::Off);
                    newState = State::stFinal;
                    }
                else if (this->m_evON)
                    {
                    this->m_evON = false;
                    m_Led.Set(LedPattern::On);
                    newState = State::stON;
                    }
                else if (this->m_evOFF)
                    {
                    this->m_evOFF = false;
                    m_Led.Set(LedPattern::Off);
                    m_Catena.SafePrintf("LED is already in OFF State!\n");
                    // stay in this state.
                    }
                else
                    {
                    // stay in this state.
                    }
                break;

            case State::stON:
                if (fBlink)
                    {
                    this->setBlink(false);
                    }
                if (this->m_evShutdown)
                    {
                    m_Led.Set(LedPattern::Off);
                    newState = State::stOFF;
                    }
                else if (this->m_evON)
                    {
                    this->m_evON = false;
                    m_Led.Set(LedPattern::On);
                    // stay in this state.
                    m_Catena.SafePrintf("LED is already in ON State!\n");
                    }
                else if (this->m_evOFF)
                    {
                    this->m_evOFF = false;
                    m_Led.Set(LedPattern::Off);
                    newState = State::stOFF;
                    }
                else
                    {
                    // stay in this state.
                    }
                break;

            case State::stFinal:
                if (fBlink)
                    {
                    m_Catena.SafePrintf("LED stopped!\n");
                    this->m_fRunning = false;
                    }
                else
                    {
                    m_Catena.SafePrintf("stFinal but not fBlink shouldn't happen.\n");
                    }
                // stay in this state.
                break;

            default:
                // the default means unknown state.
                // transition to LOW.
                m_Catena.SafePrintf("unknown state %u!\n", static_cast<unsigned>(currentState));
                newState = State::stOFF;
                break;
                }

        return newState;
        }

    // the Catena reference
    Catena &m_Catena;
    StatusLed &m_Led;

    // the event flags:

    // The LED is made ON
    bool m_evON : 1;

    // The LED is made OFF
    bool m_evOFF : 1;

    // a shutdown has been requested
    bool m_evShutdown : 1;

    // state flag: true iff the FSM is running.
    bool m_fRunning: 1;
    };

/****************************************************************************\
|
|   The variables
|
\****************************************************************************/

// instantiate the global object for the platform.
Catena gCatena;

// instantiate the LED object
StatusLed gLed (Catena::PIN_STATUS_LED);

// instantiate the BlinkLED
BlinkLED gBlinkLED (gCatena, gLed);

// forward reference to the command function
cCommandStream::CommandFn cmdOn;
cCommandStream::CommandFn cmdOff;
cCommandStream::CommandFn cmdBegin;
cCommandStream::CommandFn cmdEnd;

// the individual commmands are put in this table
static const cCommandStream::cEntry sMyExtraCommmands[] =
    {
    { "begin", cmdBegin },
    { "on", cmdOn },
    { "off", cmdOff },
    { "end", cmdEnd },
    // other commands go here....
    };

static cCommandStream::cDispatch
sMyExtraCommands_top(
        sMyExtraCommmands,          /* this is the pointer to the table */
        sizeof(sMyExtraCommmands),  /* this is the size of the table */
        nullptr                     /* this is no "first word" for all the commands in this table */
        );

/****************************************************************************\
|
|   The code
|
\****************************************************************************/

// setup is called once.
void setup()
    {
    gCatena.begin();

    /* wait 2 seconds */
    auto const now = millis();
    while (millis() - now < 2000)
        /* wait */;

    /* wait for a UART connection */
    while (! Serial)
        /* wait */;

    /* add our application-specific commands */
    gCatena.addCommands(
        /* name of app dispatch table, passed by reference */
        sMyExtraCommands_top,
        /*
        || optionally a context pointer using static_cast<void *>().
        || normally only libraries (needing to be reentrant) need
        || to use the context pointer.
        */
        nullptr
        );

    gCatena.SafePrintf("This is the FSM demo program for LED Blink.\n");

    gLed.begin();
    gCatena.registerObject(&gLed);
    }

// loop is called repeatedly.
void loop()
    {
    // Tn this app, all we have to do is invoke the Catena
    // polling framework. Everytng wlse will be driven from that.
    gCatena.poll();
    }

/****************************************************************************\
|
|   The commands -- called automatically from the framework after receiving
|   and parsing a command from the Serial console.
|
\****************************************************************************/

/* process "on" -- args are ignored */
// argv[0] is "on"
// argv[1..argc-1] are the (ignored) arguments
cCommandStream::CommandStatus cmdOn(
    cCommandStream *pThis,
    void *pContext,
    int argc,
    char **argv
    )
    {
    pThis->printf("%s\n", argv[0]);
    gBlinkLED.evON();

    return cCommandStream::CommandStatus::kSuccess;
    }

/* process "off" -- args are ignored */
// argv[0] is "off"
// argv[1..argc-1] are the (ignored) arguments
cCommandStream::CommandStatus cmdOff(
    cCommandStream *pThis,
    void *pContext,
    int argc,
    char **argv
    )
    {
    pThis->printf("%s\n", argv[0]);
    gBlinkLED.evOFF();

    return cCommandStream::CommandStatus::kSuccess;
    }

/* process "begin" -- args are ignored */
// argv[0] is "begin"
// argv[1..argc-1] are the (ignored) arguments
cCommandStream::CommandStatus cmdBegin(
    cCommandStream *pThis,
    void *pContext,
    int argc,
    char **argv
    )
    {
    pThis->printf("%s\n", argv[0]);
    gBlinkLED.begin();

    return cCommandStream::CommandStatus::kSuccess;
    }

/* process "end" -- args are ignored */
// argv[0] is "end"
// argv[1..argc-1] are the (ignored) arguments
cCommandStream::CommandStatus cmdEnd(
    cCommandStream *pThis,
    void *pContext,
    int argc,
    char **argv
    )
    {
    pThis->printf("%s\n", argv[0]);
    gBlinkLED.end();

    return cCommandStream::CommandStatus::kSuccess;
    }