/* 
 * team-split2/main.cpp 
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

  dash::Team& t = dash::Team::All();

  // create a binary tree of teams
  while( t.bottom().size()>2 ) {
    t.bottom().split(2);
  }

  cout<<myid<<":";
  for( int i=0; ; i++ ) {
    dash::Team& t = dash::Team::All().sub(i);
    cout<<t.myid()<<" ";
    
    if( t.isLeaf() )
      break;
  }
  cout<<endl;

  dash::finalize();
}
