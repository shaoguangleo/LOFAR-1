//#  GCF_Answer.h:  Callback wrapper class for GCFMyPropertySet, GCFMyProperty, 
//#                 GCFExtPropertySet, GCFExtProperty
//#
//#  Copyright (C) 2002-2003
//#  ASTRON (Netherlands Foundation for Research in Astronomy)
//#  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//#
//#  This program is free software; you can redistribute it and/or modify
//#  it under the terms of the GNU General Public License as published by
//#  the Free Software Foundation; either version 2 of the License, or
//#  (at your option) any later version.
//#
//#  This program is distributed in the hope that it will be useful,
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//#  GNU General Public License for more details.
//#
//#  You should have received a copy of the GNU General Public License
//#  along with this program; if not, write to the Free Software
//#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//#
//#  $Id$

#ifndef GCF_ANSWER_H
#define GCF_ANSWER_H

#include <GCF/GCF_TMProtocols.h>
#include <GCF/GCF_Defines.h>
#include <GCF/GCF_Event.h>

/**
 * This is an abstract class, which provides the possibility to handle 
 * asynchronous PML actions (like load APC or get property) and value changed 
 * indications (after subscription).
 * 
 * A specialisation of this class can be given to the following classes to 
 * handle their responses or the value changed indications in a generic way:
 * GCFMyPropertySet, GCFMyProperty, GCFExtPropertySet, GCFExtProperty
 * 
 * All responses or indications (events) will be given in the handleAnswer 
 * method based on the GCFEvent structure. This provides the flexibility to 
 * handle events in a state machine of a task, to skip or to handle in a 
 * different way.
*/

class GCFPValue;

class GCFAnswer
{
  public:
    GCFAnswer () {;}
    virtual ~GCFAnswer () {;}
    
    /** This is the abstract callback method, which will be invoked
      * by one of the following classes (if an instance of this class is set to 
      * one of their objects):
      * GCFMyPropertySet, GCFMyProperty, GCFExtPropertySet, GCFExtProperty
      * There are many ways to fill in this handling method, but one of the most 
      * likely ways is to forward this answer message to a task context. See the 
      * following example (see for more examples in the GCF Test suite code).
      * @code
      *   class Answer : public GCFAnswer
      *   {
      *     public:
      *       Answer(GCFTask& t) : _t(t) {;}
      *       ~Answer() {;}
      * 
      *       void handleAnswer(GCFEvent& answer);
      *       {
      *         // The port can be a dummy port @see GCFFsm
      *         // or a port of the task _t
      *         _t.dispatch(answer, aPort); 
      *       }
      *   
      *     private:    
      *       GCFTask& _t;
      *   };
      *   // If aState is the current state this method will be called indirect
      *   // by the dispatch method call.  
      *   GCFEvent::TResult Application::aState(GCFEvent& e, GCFPortInterface& p)
      *   {
      *     ...
      *     switch (e.signal)
      *     {
      *       case F_ENTRY:
      *         break;
      *   
      *       case F_MYPS_LOADED:
      *       {
      *         // The event always sould be casted by the static_cast operator
      *         GCFMYPropAnswerEvent* pResponse = static_cast<GCFMYPropAnswerEvent*>(&e);
      *         assert(pResponse);
      *         if ((strcmp(pResponse->pScope, propertySetB1.scope) == 0) &&
      *             (pResponse->result == GCF_NO_ERROR))
      *         {
      *           ...
      *         }
      *         break;
      *       }
      *     }
      *   }
      * @endcode
      * @param answer could be one of the special event structs, which 
      *               transports the respones/indication data. Note that almost 
      *               all parameters of these events are pointers to data, which 
      *               is created in and managed by PML
      */
    virtual void handleAnswer (GCFEvent& answer) = 0;
    
  private:
    //@{ 
    /// Copy contructors. Don't allow copying this object.
    GCFAnswer (const GCFAnswer&);
    GCFAnswer& operator= (const GCFAnswer&);  
    //@}
};

/// NOTE: These structs cannot be used to send messages by real port 
/// implementations like TCP. 
struct GCFPropValueEvent : public GCFEvent
{
  /// @param sig can only be F_VCHANGEMSG, F_VGETRESP
  GCFPropValueEvent(unsigned short sig) : GCFEvent(sig)
  {
      length = sizeof(GCFPropValueEvent);
  }
  const GCFPValue* pValue; ///< Pointer to the changed value object
  const char* pPropName;   ///< Pointer to the string of the property name
  bool internal;           ///< Indicates whether the internal/owned/my 
                           ///< (GCFMyProperty) or an other (GCFExtProperty) 
                           ///< property has changed (not used with F_VGETRESP)
};

struct GCFPropAnswerEvent : public GCFEvent
{
  /// @param sig can only be F_SUBSCRIBED
  GCFPropAnswerEvent(unsigned short sig) : GCFEvent(sig)
  {
      length = sizeof(GCFPropAnswerEvent);
  }
  const char* pPropName;
};

struct GCFConfAnswerEvent : public GCFPropSetAnswerEvent
{
  /// @param sig can only be F_PS_CONFIGURED
  GCFConfAnswerEvent(unsigned short sig) : GCFPropSetAnswerEvent(sig)
  {
      length = sizeof(GCFConfAnswerEvent);
  }
  const char* pApcName; ///< Pointer to the name string of the APC (excl. path and extension)
};

struct GCFPropSetAnswerEvent : public GCFEvent
{
  /// @param sig can only be F_MYPS_ENABLED, F_MYPS_DISABLED, F_EXTPS_LOADED, F_EXTPS_UNLOADED
  GCFPropSetAnswerEvent(unsigned short sig) : GCFEvent(sig)
  {
      length = sizeof(GCFPropSetAnswerEvent);
  }
  const char* pScope;   ///< Scope of the propertyset
  TGCFResult result;    ///< Result of the requested action: 
                        ///<    GCF_MYPS_ENABLE_ERROR, GCF_MYPS_DISABLE_ERROR, 
                        ///<    GCF_EXTPS_LOAD_ERROR, GCF_EXTPS_UNLOAD_ERROR, GCF_PS_CONFIGURE_ERROR
};

enum {
  F_PML_PROTOCOL = F_GCF_PROTOCOL + 2,
};
/**
 * F_PML_PROTOCOL signals
 */
enum {
  F_SUBSCRIBED_ID = 1,                                 
  F_UNSUBSCRIBED_ID,      
  F_VCHANGEMSG_ID,               
  F_VGETRESP_ID,        
  F_EXTPS_LOADED_ID,
  F_EXTPS_UNLOADED_ID,
  F_PS_CONFIGURED_ID,
  F_MYPS_ENABLED_ID,
  F_MYPS_DISABLED_ID,
};

#define F_SUBSCRIBED      F_SIGNAL(F_PML_PROTOCOL, F_SUBSCRIBED_ID,    F_IN)
#define F_UNSUBSCRIBED    F_SIGNAL(F_PML_PROTOCOL, F_UNSUBSCRIBED_ID,  F_IN)
#define F_VCHANGEMSG      F_SIGNAL(F_PML_PROTOCOL, F_VCHANGEMSG_ID,    F_IN)
#define F_VGETRESP        F_SIGNAL(F_PML_PROTOCOL, F_VGETRESP_ID,      F_IN)
#define F_EXTPS_LOADED    F_SIGNAL(F_PML_PROTOCOL, F_EXTPS_LOADED_ID,  F_IN)
#define F_EXTPS_UNLOADED  F_SIGNAL(F_PML_PROTOCOL, F_EXTPS_UNLOADED_ID,F_IN)
#define F_PS_CONFIGURED   F_SIGNAL(F_PML_PROTOCOL, F_PS_CONFIGURED_ID, F_IN)
#define F_MYPS_ENABLED    F_SIGNAL(F_PML_PROTOCOL, F_MYPS_ENABLED_ID,  F_IN)
#define F_MYPS_DISABLED   F_SIGNAL(F_PML_PROTOCOL, F_MYPS_DISABLED_ID, F_IN)

extern const char* F_PML_PROTOCOL_signalnames[];

#endif
