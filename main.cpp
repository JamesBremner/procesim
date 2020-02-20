#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <map>
#include <queue>

using namespace std;

/// resurve enumeration
enum class eResource
{
    none,
    core,
    ssd,
    tty,
};
/// event enumeration
enum class eEvent
{
    arrive,
    complete,
    endSim,
    coreFreed,
    coreRequest,
    ssdRequest,
    ttyRequest,
    free,
};

/// Resource request
class cRequest
{
public:
    eResource myRes;
    int time;
    cRequest( eResource r, int t )
        : myRes( r ),
          time( t )
    {

    }
};

/// A process
class cProcess
{
public:

    enum class eStatus
    {
        notYet,
        arrived,
        running,
        completed,
        waiting
    };
    /** CTOR
        @param[in] i id
        @param[in] a arrival time
    */
    cProcess( int i, int a )
        : id( i )
        , ar( a )
        , myNextReq( 0 )
        , myStatus( eStatus::notYet )
    {

    }
    /** Add a resource request
        @param[in] r resource required
        @param[in] t time resource required for
    */
    void Add( eResource r, int t )
    {
        vReqs.push_back( cRequest( r, t ));
    }
    /// Get current request
    cRequest& Request();

    /// Move on to next request
    void NextRequest();

    void set( eStatus s )
    {
        //cout << "Process set " << id <<" " << (int)s << "\n";
        myStatus = s;
    }
    string text();

private:
    int id;                         ///< id
    int ar;                         ///< arrival time
    vector< cRequest > vReqs;       ///< requests from this process
    int myNextReq;                  ///< index of next request
    eStatus myStatus;
};

/// The processes
class cProcessTable
{
public:

    /** Add a process
        @param[in] id
        @param[in] t arrival time
    */
    void Add( int id, int t )
    {
        myProcess.insert( make_pair(id, cProcess( id, t )));
    }
    /** Add a request from the process
        @param[in] id
        @param[in] r resource
        @param[in] t time the resource is required
    */
    void Add( int id, eResource r, int t )
    {
        myProcess.find( id )->second.Add( r, t );
    }
    /** Current Request for process
        @param[in] id
    */
    cRequest& Request( int id );

    cProcess& find( int id );

    string text();

private:
    map< int,cProcess > myProcess;
};

///  A simulated event
class cEvent
{
public:
    eEvent myType;          ///< the type of event
    int myPID;              ///< the process ID causing the event

    /** CTOR
        @param[in] e event type
        @param[in] p process id
    */
    cEvent( eEvent e, int p )
        : myType( e )
        , myPID( p )
    {

    }
    /// Human readable text desribing event as it completes
    string text();

    void execute();

};

/// A schedule of events waiting to happen
class cSchedule
{
public:

    /** Add an event
        @param[in] time  the time of the event
        @param[in] e the event
    */
    void Add( int time, const cEvent& e )
    {
        mySchedule.insert( make_pair( time, e ));
    }
    /** Get next event that will occur
        @param[in/out] Clock simulation time
        @return the next event
        The simulation clock will be advanced to the next event
    */
    cEvent Next( int& Clock );

    /// Remove completed event from schedule
    void Done()
    {
        mySchedule.erase( mySchedule.begin() );
    }
    string text();
private:
    /// Upcoming events mapped by their times
    multimap< int,cEvent > mySchedule;
};

/// The cores
class cCores
{
public:

    /// CTOR - single core
    cCores()
        : myCoreCount( 1 )
        , myCoreBusy( 0 )
        , myBusyStart( 1 )
        , myBusyTotal( 1 )
    {

    }
    // Specify number of cores
    void set( int count )
    {
        myCoreCount = count;
        myBusyStart.resize( count );
        myBusyTotal.resize( count );
    }
    /** Request a core
        @return true on success, false on no free core available
    */
    bool Request();

    /// Free a core
    void Free();

    vector<int> Utilization();

private:
    int myCoreCount;                ///< number of cores
    int myCoreBusy;                 ///< number of bust cores
    vector<int> myBusyStart;        ///< time each core started being busy
    vector<int> myBusyTotal;        ///< total time each core was busy
};

/// The simulator
class cProcessorSimulator
{
public:
    /** read specification from standard input
    <pre>
    p pid art   define process with is and arival time
    c time      request core for time
    s time      request ssd for time
    t time      reuest tty for time
    </pre>
    */
    void Read();

    /// run the simulation
    void Run();

    /// A process has arrived
    void Arrive( int pid );

    /// A process has requested a core
    void RequestCore( int pid );

    /// A process has freed a core
    void FreeCore();

    /// A process has requested ssd or tty
    void Request( int pid );

    /// find process from process id
    cProcess& find( int pid );

    /// Current event is complete - remove from schedule
    void EventDone()
    {
        mySchedule.Done();
    }
    /// get current simulation time
    int time()
    {
        return myTime;
    }

    /// human readable snapshot string
    string snapShot();

    /// get core utilization per centages
    vector<int> CoreUtilization()
    {
        return myCores.Utilization();
    }

private:

    // queue of processes waiting for core
    queue<int> myQueue;

    // the cores
    cCores myCores;

    // the process table
    cProcessTable myProcessTable;

    // simulation clock
    int myTime;

    // schedule of waiting events
    cSchedule mySchedule;
};


cProcessorSimulator theSim;

void cProcessorSimulator::Arrive( int pid )
{
    cProcess& P = theSim.find( pid );
    P.set( cProcess::eStatus::arrived );
    switch( P.Request( ).myRes )
    {
    case eResource::core:
        mySchedule.Add( myTime, cEvent( eEvent::coreRequest, pid ));
        break;
    case eResource::ssd:
        mySchedule.Add( myTime, cEvent( eEvent::ssdRequest, pid ));
        break;
    case eResource::tty:
        mySchedule.Add( myTime, cEvent( eEvent::ttyRequest, pid ));
        break;
    }
}

void cProcessorSimulator::Request( int pid )
{
    cProcess& P = theSim.find( pid );
    mySchedule.Add(
        myTime + P.Request().time,
        cEvent( eEvent::free, pid ));
    P.set( cProcess::eStatus::running );
}

void cProcessorSimulator::RequestCore( int pid )
{
    cProcess& P = theSim.find( pid );
    // request a core
    if( myCores.Request() )
    {
        // successful - schedule event when the core will be freed
        mySchedule.Add(
            myTime + P.Request().time,
            cEvent( eEvent::coreFreed, pid ));
        P.set( cProcess::eStatus::running );
    }
    else
    {
        // no core available - add process to queue
        myQueue.push( pid );
        P.set( cProcess::eStatus::waiting );
    }
}

void cProcessorSimulator::FreeCore()
{
    // free a core
    myCores.Free();

    // check for process waiting for core
    if( myQueue.empty() )
        return;

    // id of waiting process
    int pid = myQueue.front();

    // remove process from queue
    myQueue.pop();

    // request a core ( must succeed because we just freed a core )
    myCores.Request();

    // schedule event when the core will be freed
    mySchedule.Add(
        myTime + myProcessTable.Request( pid ).time,
        cEvent( eEvent::coreFreed, pid ));

}

cProcess& cProcessorSimulator::find( int pid )
{
    return myProcessTable.find( pid );
}
string cProcessorSimulator::snapShot()
{
    stringstream ss;
    //ss << mySchedule.text();
    ss << myProcessTable.text();
    ss << myQueue.size() << " processes waiting for core\n";
    ss << "\n";
    return ss.str();
}

bool cCores::Request()
{
    //cout << "Core request " << myCoreBusy << " busy of " << myCoreCount << "\n";
    if ( myCoreBusy == myCoreCount )
        return false;
    myBusyStart[ myCoreBusy ] = theSim.time();
    myCoreBusy++;
    return true;
}

void cCores::Free()
{
    if( myCoreBusy <= 0 )
        throw std::runtime_error("cCore undeflow");
    myCoreBusy--;
    myBusyTotal[ myCoreBusy] += theSim.time() - myBusyStart[ myCoreBusy ];
}

vector<int> cCores::Utilization()
{
    vector<int> u;
    for( int c = 0; c < myCoreCount; c++ )
        u.push_back( 100 * myBusyTotal[c] / theSim.time() );
    return u;
}

cEvent cSchedule::Next( int& Clock )
{
    if( ! mySchedule.size() )
    {
        // there are no more events,
        // so return the simulation end event
        static cEvent e( eEvent::endSim, -1);
        return e;
    }
    Clock = mySchedule.begin()->first;
    // tell user what has happened
    cEvent E = mySchedule.begin()->second;
    return E;
}

std::string cSchedule::text()
{
    stringstream ss;
    ss << "==Schedule==\n";
    for( auto it = mySchedule.begin();
            it != mySchedule.end();
            it++ )
    {
        ss << it->first << " " << it->second.text() << "\n";
    }
    ss << "=========\n";
    return ss.str();
}

cRequest& cProcess::Request()
{
    if( myNextReq >= (int)vReqs.size() )
    {
        static cRequest nullreq( eResource::none, -1 );
        return nullreq;
    }
    return vReqs[ myNextReq ];
}

void cProcess::NextRequest()
{
    if( myNextReq == (int)vReqs.size()-1 )
        myStatus = eStatus::completed;
    else
        myNextReq++;
}
string cProcess::text()
{
    stringstream ss;
    ss << id << " ";
    switch( myStatus )
    {
    case eStatus::notYet:
        ss << "---";
        break;
    case eStatus::arrived:
        ss << "arrived";
        break;
    case eStatus::running:
        ss << "running";
        break;
    case eStatus::waiting:
        ss << "waiting";
        break;
    case eStatus::completed:
        ss << "completed";
        break;
    }
    return ss.str();
}

cRequest& cProcessTable::Request( int id )
{
    auto it = myProcess.find( id );
    if( it == myProcess.end() )
        throw std::runtime_error("lost process");
    return it->second.Request();
}
cProcess& cProcessTable::find( int id )
{
    auto it = myProcess.find( id );
    if( it == myProcess.end() )
        throw std::runtime_error("lost process");
    return it->second;
}

string cProcessTable::text()
{
    stringstream ss;
    ss << "++ Process Table ++\n";
    for( auto pit : myProcess )
        ss << pit.second.text() << "\n";
    ss << "++++++++++\n";
    return ss.str();
}

string cEvent::text()
{
    stringstream ss;
    string desc;
    switch( myType )
    {
    case eEvent::arrive:
        desc = " arrives";
        break;
    case eEvent::complete:
        desc =  " completes";
        break;
    case eEvent::endSim:
        return "Simulation completed";
        break;
    case eEvent::coreRequest:
        desc =  " requests core";
        break;
    case eEvent::ssdRequest:
        desc =  " requests ssd";
        break;
    case eEvent::ttyRequest:
        desc =  " requests tty";
        break;
    case eEvent::coreFreed:
        desc =  " frees core";
        break;
    default:
        return "";
    }
    ss << "Process " << myPID << desc;
    return ss.str();
}

void cEvent::execute()
{
    cout << "Event: " << text() << "\n";
    cProcess& P = theSim.find( myPID );

    switch( myType )
    {
    case eEvent::arrive:
        theSim.Arrive( myPID );
        break;

    case eEvent::coreRequest:
        theSim.RequestCore( myPID );
        break;

    case eEvent::coreFreed:
        P.NextRequest();
        theSim.FreeCore();
        break;

    case eEvent::ssdRequest:
    case eEvent::ttyRequest:
        theSim.Request( myPID );
        break;

    case eEvent::free:
        P.NextRequest();
        break;
    }
    theSim.EventDone();
}

void cProcessorSimulator::Read()
{
    string line;
    int pid;
    while( getline( cin, line ))
    {
        if( ! line.length() )
            continue;
        cout << line << "\n";
        char * p = (char*)line.c_str() + line.find(" ")+1;
        switch( line[0] )
        {
        case 'p':
        {
            // process

            pid = strtol(p,&p,10);
            int arv = strtol(p,NULL,10);
            cout << "pid " << pid << " arrive " << arv << endl;
            myProcessTable.Add( pid, arv );
            mySchedule.Add( arv, cEvent( eEvent::arrive, pid ) );
        }
        break;

        case 'c':
            myProcessTable.Add(
                pid,
                eResource::core,
                strtol(p,NULL,10) );
            break;
        case 't':
            myProcessTable.Add(
                pid,
                eResource::tty,
                strtol(p,NULL,10) );
            break;
        case 's':
            myProcessTable.Add(
                pid,
                eResource::ssd,
                strtol(p,NULL,10) );
            break;
        }
    }
}

void cProcessorSimulator::Run()
{
    cout << theSim.snapShot();

    // start the clock
    myTime = 0;

    // run simulation until all reuests are completed
    while( 1 )
    {
        cEvent E = mySchedule.Next( myTime );

        // check for completed simulation
        if( E.myType == eEvent::endSim )
            break;

        E.execute();

        cout << theSim.snapShot();
    }

    cout << "Simulation ended at " << myTime << "\n";
}

int main()
{
    theSim.Read();
    theSim.Run();
    cout << "Core Utilization %\n";
    for( auto u : theSim.CoreUtilization() )
    {
        cout << u << "\n";
    }
    return 0;
}
