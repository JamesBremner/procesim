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
    int myCoreCount;
    int myCoreBusy;
    cCores()
        : myCoreCount( 1 )
        , myCoreBusy( 0 )
    {

    }
    void set( int count )
    {
        myCoreCount = count;
    }
    bool Request()
    {
        //cout << "Core request " << myCoreBusy << " busy of " << myCoreCount << "\n";
        if ( myCoreBusy == myCoreCount )
            return false;
        myCoreBusy++;
        return true;
    }
    void Free()
    {
        if( myCoreBusy <= 0 )
            throw std::runtime_error("cCore undeflow");
        myCoreBusy--;
    }
};

/// The simulator
class cProcessorSimulator
{
public:
    /// read specification from standard input
    void Read();

    /// A process has arrived
    void Arrive( int pid );

    /// A process has requested a core
    void RequestCore( int pid );

    /// A process has freed a core
    void FreeCore();

    /// find process from process id
    cProcess& find( int pid );

    /// human readable snapshot string
    string snapShot();

private:

    // queue of processes waiting for core
    queue<int> myQueue;

    // the cores
    cCores myCores;

    // the process table
    cProcessTable myProcessTable;
};


cSchedule theSchedule;
int theTime;

cProcessorSimulator theSim;

void cProcessorSimulator::Arrive( int pid )
{
    cProcess& P = theSim.find( pid );
    P.set( cProcess::eStatus::arrived );
    switch( P.Request( ).myRes )
    {
    case eResource::core:
        theSchedule.Add( theTime, cEvent( eEvent::coreRequest, pid ));
        break;
    }
}

void cProcessorSimulator::RequestCore( int pid )
{
    cProcess& P = theSim.find( pid );
    // request a core
    if( myCores.Request() )
    {
        // successful - schedule event when the core will be freed
        theSchedule.Add(
            theTime + P.Request().time,
            cEvent( eEvent::coreFreed, pid ));
        P.set( cProcess::eStatus::running );
    }
    else
    {
        // no core available - add process to queue
        cout << "process blocked\n";
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
    theSchedule.Add(
        theTime + myProcessTable.Request( pid ).time,
        cEvent( eEvent::coreFreed, pid ));

}
cProcess& cProcessorSimulator::find( int pid )
{
    return myProcessTable.find( pid );
}
string cProcessorSimulator::snapShot()
{
    stringstream ss;
    //ss << theSchedule.text();
    ss << myProcessTable.text();
    ss << myQueue.size() << " processes waiting for core\n";
    ss << "\n";
    return ss.str();
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
    switch( myType )
    {
    case eEvent::arrive:
        ss << "Process " << myPID << " arrives";
        break;
    case eEvent::complete:
        ss << "Process " << myPID << " completes";
        break;
    case eEvent::endSim:
        ss << "Simulation completed";
        break;
    case eEvent::coreRequest:
        ss << "Process " << myPID << " requests core";
        break;
    case eEvent::coreFreed:
        ss << "Process " << myPID << " frees core";
        break;
    }
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
    }
    theSchedule.Done();
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
        eResource r;
        if( line[0] == 'p' )
        {
            // process

            pid = strtol(p,&p,10);
            int arv = strtol(p,NULL,10);
            cout << "pid " << pid << " arrive " << arv << endl;
            myProcessTable.Add( pid, arv );
            theSchedule.Add( arv, cEvent( eEvent::arrive, pid ) );
        }
        else if( line[0] == 'c' )
        {
            r = eResource::core;
            myProcessTable.Add( pid, r, strtol(p,NULL,10) );
        }
    }
}

void Run()
{
    cout << theSim.snapShot();

    // start the clock
    theTime = 0;

    // run simulation until all reuests are completed
    while( 1 )
    {
        cEvent E = theSchedule.Next( theTime );

        // check for completed simulation
        if( E.myType == eEvent::endSim )
            break;

        E.execute();

        cout << theSim.snapShot();
    }

    cout << "Simulation ended at " << theTime << "\n";
}

int main()
{
    theSim.Read();
    Run();
    return 0;
}
