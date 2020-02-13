#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <map>

using namespace std;

enum class eResource
{
    none,
    core,
    ssd,
    tty,
};
enum class eEvent
{
    arrive,
    complete,
    endSim,
};
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
class cProcess
{
public:
    int id;
    int ar;
    vector< cRequest > vReqs;
    int myNextReq;
    cProcess( int i, int a )
        : id( i )
        , ar( a )
        , myNextReq( 0 )
    {

    }
    void Add( eResource r, int t )
    {
        vReqs.push_back( cRequest( r, t ));
    }
    cRequest NextRequest()
    {
        if( myNextReq >= (int)vReqs.size() )
        {
            return cRequest( eResource::none, -1 );
        }
        return vReqs[ myNextReq++ ];
    }
};

class cEvent
{
public:
    eEvent myType;
    int myPID;
    cEvent( eEvent e, int p )
        : myType( e )
        , myPID( p )
    {

    }
    string text()
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
};

class cSchedule
{
public:
   multimap< int,cEvent > mySchedule;

   void Add( int time, const cEvent& e )
   {
       mySchedule.insert( make_pair( time, e ));
   }
   cEvent Next( int& Clock )
   {
        auto it = mySchedule.lower_bound( Clock );
        if( it == mySchedule.end()) {
            static cEvent end_event( eEvent::endSim, -1 );
            return end_event;
        }
        Clock += it->first;
       // mySchedule.erase( it );
        return it->second;
   }
};

map< int,cProcess > mProcess;
cSchedule theSchedule;

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
            mProcess.insert( make_pair(pid, cProcess( pid, arv )));
            theSchedule.Add( arv, cEvent( eEvent::arrive, pid ) );
        }
        else if( line[0] == 'c' )
        {
            r = eResource::core;
            mProcess.find( pid )->second.Add( r, strtol(p,NULL,10) );
        }
    }
}

void Run()
{
    // start the clock
    int theTime = 0;

    // run simulation until all reuests are completed
    while( 1 )
    {
        // find next event on schedule
        auto it = theSchedule.mySchedule.lower_bound( theTime);
        if( it == theSchedule.mySchedule.end())
            return;

        // advance clock to next event
        theTime = it->first;

        //cEvent E = theSchedule.Next( theTime );

        // tell user what has happened
        cout << it->second.text() << " at " << theTime << "\n";

        if( it->second.myType == eEvent::endSim )
            break;

        // does the process have another request?
        int pid = it->second.myPID;
        auto req = mProcess.find( pid )->second.NextRequest();
        if( req.myRes != eResource::none ) {

            // enter the new request into the schedule
            theSchedule.Add( theTime+req.time, cEvent( eEvent::complete, pid ));
        }

        // remove the completed event from the schedule
        theSchedule.mySchedule.erase( it );
    }
}

int main()
{
    Read();
    Run();
    return 0;
}
