/* 
 * team-split/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */


#include<iostream>
#include<libdash.h>

int main(int argc, char* argv[]) 
{
  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto size = dash::size();

  dash::Team& t0 = dash::Team::All();
  dash::Team& t1 = t0.split(2);
  dash::Team& t2 = t1.split(2);

  std::cout << myid << "/" << size << ": "
            << t0.position() << ":[" << t0.myid() << "," << t0.size() <<"] "
            << t1.position() << ":[" << t1.myid() << "," << t1.size() <<"] "
            << t2.position() << ":[" << t2.myid() << "," << t2.size() <<"] "
            << std::endl;

  dash::finalize();
}
