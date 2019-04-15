/*******************************************************************************
 * ARICPP - ARI interface for C++
 * Copyright (C) 2017 Daniele Pallastrelli
 *
 * This file is part of aricpp.
 * For more information, see http://github.com/daniele77/aricpp
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/


#ifndef ARICPP_CHANNEL_H_
#define ARICPP_CHANNEL_H_

#include <string>
#include "client.h"
#include "proxy.h"
#include "terminationdtmf.h"
#include "recording.h"
#include "playback.h"
#include "urlencode.h"

namespace aricpp
{

class Channel
{
public:

    enum class State
    {
        down,
        reserved,
        offhook,
        dialing,
        ring,
        ringing,
        up,
        busy,
        dialingoffhook,
        prering,
        mute,
        unknown
    };

    ///////////////////////////////////////////////////////////////
    // Direction smart enum

    // all this machinery to initialize static members in the header file

    class Direction; // forward declaration

    template<class Dummy>
    struct DirectionBase
    {
        static const Direction none;
        static const Direction both;
        static const Direction in;
        static const Direction out;
    };

    class Direction : public DirectionBase<void>
    {
    public:
        operator std::string() const { return value; }
    private:
        friend struct DirectionBase<void>;
        Direction(const char* v) : value(v) {}
        const std::string value;
    };

    ///////////////////////////////////////////////////////////////


    Channel(const Channel& rhs) = delete;
    Channel& operator=(const Channel& rhs) = delete;
    Channel(Channel&& rhs) = default;
    Channel& operator=(Channel&& rhs) = default;
    
    ~Channel()
    {
        Hangup();
    }

    Channel(Client& _client, const std::string _id, const std::string& _state = {}) :
        id(_id), client(&_client)
    {
        StateChanged(_state);
    }

    Proxy& Ring() const
    {
        return Proxy::Command(Method::post, "/ari/channels/"+id+"/ring", client);
    }

    Proxy& RingStop() const
    {
        return Proxy::Command(Method::delete_, "/ari/channels/"+id+"/ring", client);
    }

    Proxy& Mute(Direction dir=Direction::both) const
    {
        return Proxy::Command(
            Method::post,
            "/ari/channels/" + id + "/mute?"
            "direction=" + static_cast<std::string>(dir),
            client
        );
    }

    Proxy& Unmute(Direction dir=Direction::both) const
    {
        return Proxy::Command(
            Method::delete_,
            "/ari/channels/" + id + "/mute?"
            "direction=" + static_cast<std::string>(dir),
            client
        );
    }

    Proxy& Hold() const
    {
        return Proxy::Command(Method::post, "/ari/channels/"+id+"/hold", client);
    }

    Proxy& Unhold() const
    {
        return Proxy::Command(Method::delete_, "/ari/channels/"+id+"/hold", client);
    }

    Proxy& Silence() const
    {
        return Proxy::Command(Method::post, "/ari/channels/"+id+"/silence", client);
    }

    Proxy& StopSilence() const
    {
        return Proxy::Command(Method::delete_, "/ari/channels/"+id+"/silence", client);
    }

    Proxy& StartMoh(const std::string& mohClass={}) const
    {
        std::string query = "/ari/channels/"+id+"/moh";
        if (!mohClass.empty()) query += "?mohClass=" + mohClass;
        return Proxy::Command(Method::post, std::move(query), client);
    }

    Proxy& StopMoh() const
    {
        return Proxy::Command(Method::delete_, "/ari/channels/"+id+"/moh", client);
    }

    Proxy& Answer() const
    {
        return Proxy::Command(Method::post, "/ari/channels/"+id+"/answer", client);
    }

    Proxy& Hangup() const
    {
        return Proxy::Command(Method::delete_, "/ari/channels/"+id, client);
    }

    /**
     * @brief Create an asterisk channel and dial
     *
     * @param endpoint Endpoint to call (e.g., pjsip/100)
     * @param application The stasis application that is subscribed to the originated channel.
     *                    When the channel is answered, it will be passed to this Stasis application.
     * @param callerId CallerID to use for the call
     * @param variables Holds variable key/value pairs to set on the channel on creation
                        (e.g., {"CALLERID(name)":"Alice", "VAR2":"Value"} )
     * @return Proxy& You can call After() and OnError() on the returned object
     */
    Proxy& Call(
        const std::string& endpoint,
        const std::string& application,
        const std::string& callerId,
        std::string variables={}
    ) const
    {
        if (!variables.empty()) variables = "{\"variables\":"+variables+"}";
        return Proxy::Command(
            Method::post,
            "/ari/channels?"
            "endpoint=" + UrlEncode(endpoint) +
            "&app=" + UrlEncode(application) +
            "&channelId=" + UrlEncode(id) +
            "&callerId=" + UrlEncode(callerId) +
            "&timeout=-1"
            "&appArgs=internal",
            client,
            std::move(variables) // now http body :-)
        );
    }

    /**
     * @brief Create only an asterisk channel (once created, you can dial with Channel::Dial)
     *
     * @param endpoint Endpoint to call (e.g., pjsip/100)
     * @param application The stasis application that is subscribed to the originated channel.
     *                    When the channel is answered, it will be passed to this Stasis application.
     */
    Proxy& Create(const std::string& endpoint, const std::string& application) const
    {
        return Proxy::Command(
            Method::post,
            "/ari/channels/create?"
            "endpoint=" + UrlEncode(endpoint) +
            "&app=" + UrlEncode(application) +
            "&channelId=" + UrlEncode(id) +
            "&appArgs=internal",
            client
        );
    }

    /**
     * @brief Dial an asterisk channel previously created with Channel::Create
     *
     * @return Proxy& You can call After() and OnError() on the returned object
     */
    Proxy& Dial() const
    {
        return Proxy::Command(
            Method::post,
            "/ari/channels/" + id + "/dial",
            client
        );
    }

    Proxy& Redirect(const std::string& endpoint) const
    {
        return Proxy::Command(
            Method::post,
            "/ari/channels/"+id+"/redirect?"
            "endpoint=" + UrlEncode(endpoint),
            client
        );
    }

    Proxy& SendDtmf(const std::string& dtmf, int between=-1, int duration=-1, int before=-1, int after=-1) const
    {
        return Proxy::Command(
            Method::post,
            "/ari/channels/"+id+"/dtmf?"
            "dtmf=" + UrlEncode(dtmf) +
            ( between < 0 ? "" : "&between=" + std::to_string(between) ) +
            ( duration < 0 ? "" : "&duration=" + std::to_string(duration) ) +
            ( before < 0 ? "" : "&before=" + std::to_string(before) ) +
            ( after < 0 ? "" : "&after=" + std::to_string(after) ),
            client
        );
    }

    ProxyPar<Playback>& Play(const std::string& media, const std::string& lang={},
                const std::string& playbackId={}, int offsetms=-1, int skipms=-1) const
    {
        Playback playback(client);
        return ProxyPar<Playback>::Command(
            Method::post,
            "/ari/channels/"+id+"/play?"
            "media=" + UrlEncode(media) +
            "&playbackId=" + playback.Id() +
            ( lang.empty() ? "" : "&lang=" + lang ) +
            ( playbackId.empty() ? "" : "&playbackId=" + playbackId ) +
            ( offsetms < 0 ? "" : "&offsetms=" + std::to_string(offsetms) ) +
            ( skipms < 0 ? "" : "&skipms=" + std::to_string(skipms) ),
            client,
            playback
        );
    }

    ProxyPar<Recording>& Record(const std::string& name, const std::string& format,
                  int maxDurationSeconds=-1, int maxSilenceSeconds=-1,
                  const std::string& ifExists={}, bool beep=false, TerminationDtmf terminateOn=TerminationDtmf::none) const
    {
        Recording recording(name, client);
        return ProxyPar<Recording>::Command(
            Method::post,
            "/ari/channels/"+id+"/record?"
            "name=" + UrlEncode(name) +
            "&format=" + format +
            "&terminateOn=" + static_cast<std::string>(terminateOn) +
            ( beep ? "&beep=true" : "&beep=false" ) +
            ( ifExists.empty() ? "" : "&ifExists=" + ifExists ) +
            ( maxDurationSeconds < 0 ? "" : "&maxDurationSeconds=" + std::to_string(maxDurationSeconds) ) +
            ( maxSilenceSeconds < 0 ? "" : "&maxSilenceSeconds=" + std::to_string(maxSilenceSeconds) ),
            client,
            recording
        );
    }

    Proxy& SetVar(const std::string& var, const std::string& value={}) const
    {
        std::string query = "/ari/channels/" + id + "/variable?"
                                  "variable=" + UrlEncode(var);
        if (!value.empty()) query += "&value=" + UrlEncode(value);
        return Proxy::Command(Method::post, std::move(query), client);
    }

    ProxyPar<std::string>& GetVar(const std::string& var) const
    {
        const std::string query = "/ari/channels/"+id+"/variable";
        const std::string body = "{\"variable\":\""+var+"\"}";
        return ProxyPar<std::string>::Command(Method::get, std::move(query), client, std::move(body));
    }

    Proxy& Snoop(const std::string& app, Direction spy=Direction::none, Direction whisper=Direction::none, const std::string& appArgs={}, const std::string& snoopId={}) const
    {
        return Proxy::Command(
            Method::post,
            "/ari/channels/"+id+"/snoop?"
            "app=" + app +
            "&spy=" + static_cast<std::string>(spy) +
            "&whisper=" + static_cast<std::string>(whisper) +
            ( appArgs.empty() ? "" : "&appArgs=" + appArgs ) +
            ( snoopId.empty() ? "" : "&snoopId=" + snoopId ),
            client
        );
    }

    const std::string& Id() const { return id; }

    /// Returns true if the asterisk channel has been destroyed.
    /// You can call Channel::Cause() to retrieve the Q.850 cause code.
    /// When a channel does not exist on asterisk anymore, the aricpp library
    /// doesn't own anymore a reference to the aricpp::Channel, so the object
    /// will be destroyed when the clients run out of references to it.
    bool IsDead() const { return dead; }

    /*! Returns the channel destroy cause as codified in Q.850
        see e.g., page http://support.sonus.net/display/uxapidoc/q.850+cause+codes+-+reference

        These are the current codes, based on the Q.850/Q.931
        specification:

        - AST_CAUSE_UNALLOCATED                      1
        - AST_CAUSE_NO_ROUTE_TRANSIT_NET             2
        - AST_CAUSE_NO_ROUTE_DESTINATION             3
        - AST_CAUSE_MISDIALLED_TRUNK_PREFIX          5
        - AST_CAUSE_CHANNEL_UNACCEPTABLE             6
        - AST_CAUSE_CALL_AWARDED_DELIVERED           7
        - AST_CAUSE_PRE_EMPTED                       8
        - AST_CAUSE_NUMBER_PORTED_NOT_HERE          14
        - AST_CAUSE_NORMAL_CLEARING                 16
        - AST_CAUSE_USER_BUSY                       17
        - AST_CAUSE_NO_USER_RESPONSE                18
        - AST_CAUSE_NO_ANSWER                       19
        - AST_CAUSE_CALL_REJECTED                   21
        - AST_CAUSE_NUMBER_CHANGED                  22
        - AST_CAUSE_REDIRECTED_TO_NEW_DESTINATION   23
        - AST_CAUSE_ANSWERED_ELSEWHERE              26
        - AST_CAUSE_DESTINATION_OUT_OF_ORDER        27
        - AST_CAUSE_INVALID_NUMBER_FORMAT           28
        - AST_CAUSE_FACILITY_REJECTED               29
        - AST_CAUSE_RESPONSE_TO_STATUS_ENQUIRY      30
        - AST_CAUSE_NORMAL_UNSPECIFIED              31
        - AST_CAUSE_NORMAL_CIRCUIT_CONGESTION       34
        - AST_CAUSE_NETWORK_OUT_OF_ORDER            38
        - AST_CAUSE_NORMAL_TEMPORARY_FAILURE        41
        - AST_CAUSE_SWITCH_CONGESTION               42
        - AST_CAUSE_ACCESS_INFO_DISCARDED           43
        - AST_CAUSE_REQUESTED_CHAN_UNAVAIL          44
        - AST_CAUSE_FACILITY_NOT_SUBSCRIBED         50
        - AST_CAUSE_OUTGOING_CALL_BARRED            52
        - AST_CAUSE_INCOMING_CALL_BARRED            54
        - AST_CAUSE_BEARERCAPABILITY_NOTAUTH        57
        - AST_CAUSE_BEARERCAPABILITY_NOTAVAIL       58
        - AST_CAUSE_BEARERCAPABILITY_NOTIMPL        65
        - AST_CAUSE_CHAN_NOT_IMPLEMENTED            66
        - AST_CAUSE_FACILITY_NOT_IMPLEMENTED        69
        - AST_CAUSE_INVALID_CALL_REFERENCE          81
        - AST_CAUSE_INCOMPATIBLE_DESTINATION        88
        - AST_CAUSE_INVALID_MSG_UNSPECIFIED         95
        - AST_CAUSE_MANDATORY_IE_MISSING            96
        - AST_CAUSE_MESSAGE_TYPE_NONEXIST           97
        - AST_CAUSE_WRONG_MESSAGE                   98
        - AST_CAUSE_IE_NONEXIST                     99
        - AST_CAUSE_INVALID_IE_CONTENTS            100
        - AST_CAUSE_WRONG_CALL_STATE               101
        - AST_CAUSE_RECOVERY_ON_TIMER_EXPIRE       102
        - AST_CAUSE_MANDATORY_IE_LENGTH_ERROR      103
        - AST_CAUSE_PROTOCOL_ERROR                 111
        - AST_CAUSE_INTERWORKING                   127
    */
    int Cause() const { return cause; }

    State GetState() const { return state; }

    const std::string& Name() const { return name; }
    const std::string& Extension() const { return extension; }
    const std::string& CallerNum() const { return callerNum; }
    const std::string& CallerName() const { return callerName; }

private:

    friend class AriModel;
    void StasisStart( const std::string& _name, const std::string& _ext,
                      const std::string& _callerNum, const std::string& _callerName)
    {
        name = _name;
        extension = _ext;
        callerNum = _callerNum;
        callerName = _callerName;
    }
    void StateChanged(const std::string& s)
    {
        if ( s == "Down" ) state = State::down;
        else if ( s == "Rsrvd" ) state = State::reserved;
        else if ( s == "OffHook" ) state = State::offhook;
        else if ( s == "Dialing" ) state = State::dialing;
        else if ( s == "Ring" ) state = State::ring;
        else if ( s == "Ringing" ) state = State::ringing;
        else if ( s == "Up" ) state = State::up;
        else if ( s == "Busy" ) state = State::busy;
        else if ( s == "Dialing Offhook" ) state = State::dialingoffhook;
        else if ( s == "Pre-ring" ) state = State::prering;
        else if ( s == "Mute" ) state = State::mute;
        else if ( s == "Unknown" ) state = State::unknown;
        else state = State::unknown;
    }
    void Dead(int _cause, const std::string& /*causeTxt*/)
    {
        dead=true;
        cause=_cause;
    }

    const std::string id;
    Client* client;
    bool dead = false;
    State state = State::unknown;
    std::string name;
    std::string extension;
    std::string callerNum;
    std::string callerName;
    int cause = -1;
};

template<class Dummy> const Channel::Direction Channel::DirectionBase<Dummy>::none{"none"};
template<class Dummy> const Channel::Direction Channel::DirectionBase<Dummy>::both{"both"};
template<class Dummy> const Channel::Direction Channel::DirectionBase<Dummy>::in{"in"};
template<class Dummy> const Channel::Direction Channel::DirectionBase<Dummy>::out{"out"};


/// Converts the Channel State enum class to a string
inline std::string ToString(Channel::State s)
{
    static const char* d[] =
    {
        "down",
        "reserved",
        "offhook",
        "dialing",
        "ring",
        "ringing",
        "up",
        "busy",
        "dialingoffhook",
        "prering",
        "mute",
        "unknown"
    };
    return d[ static_cast<std::underlying_type_t<Channel::State>>(s) ];
}

} // namespace aricpp

#endif
