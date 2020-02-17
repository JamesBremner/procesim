#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <map>

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

    /** CTOR
        @param[in] i id
        @param[in] a arrival time
    */
    cProcess( int i, int a )
        : id( i )
        , ar( a )
        , myNextReq( 0 )
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
    /// Execute next reuest
    void NextRequest();

private:
    int id;                         ///< id
    int ar;                         ///< arrival time
    vector< cRequest > vReqs;       ///< requests from this process
    int myNextReq;                  ///< index of next request
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
    /** Execute next process request
        @param[in] id
    */
    void ExecNextRequest( int id );

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
    cEvent Next( int& Clock )
    {
        if( ! mySchedule.size() )
        {
            // there are no more event,
            // so return the simulation end event
            static cEvent e( eEvent::endSim, -1);
            return e;
        }
        Clock = mySchedule.begin()->first;
        return mySchedule.begin()->second;
    }
    /// Remove completed event from schedule
    void Done()
    {
        mySchedule.erase( mySchedule.begin() );
    }
private:
    /// Upcoming events mapped by their times
    multimap< int,cEvent > mySchedule;
};

cProcessTable theProcessTable;
cSchedule theSchedule;
int theTime;

void cProcess::NextRequest()
{
    if( myNextReq >= (int)vReqs.size() )
    {
        return;
    }

    cRequest& req = vReqs[ myNextReq++ ];
    if( req.myRes == eResource::none )
        return;


    // enter the new request into the schedule
    theSchedule.Add( theTime+req.time, cEvent( eEvent::complete, id ));

}

void cProcessTable::ExecNextRequest( int id )
{
    myProcess.find( id )->second.NextRequest();
    // remove the completed event from the schedule
    theSchedule.Done();
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
    }
    return ss.str();
}

void Read()
{
    string line;
    int pid;
    while( getline( cin, line ))
    {
        if( ! line.length() )
            continue;
        //cout << line << "\n";
        char * p = (char*)line.c_str() + line.find(" ")+1;
        eResource r;
        if( line[0] == 'p' )
        {
            // process

            pid = strtol(p,&p,10);
            int arv = strtol(p,NULL,10);
            //cout << "pid " << pid << " arrive " << arv << endl;
            theProcessTable.Add( pid, arv );
            theSchedule.Add( arv, cEvent( eEvent::arrive, pid ) );
        }
        else if( line[0] == 'c' )
        {
            r = eResource::core;
            theProcessTable.Add( pid, r, strtol(p,NULL,10) );
        }
    }
}

void Run()
{
    // start the clock
    theTime = 0;

    // run simulation until all reuests are completed
    while( 1 )
    {
        cEvent E = theSchedule.Next( theTime );

        // tell user what has happened
        cout << E.text() << " at " << theTime << "\n";

        // check for completed simulation
        if( E.myType == eEvent::endSim )
            break;

        // do the process' next request
        theProcessTable.ExecNextRequest( E.myPID );
    }
}

int main()
{
    Read();
    Run();
    return 0;
}
