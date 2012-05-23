/*-------------------------------------------
 * 
 *        nw.c++ for program nw
 *        taken from http://www.rolfmuertter.com/code/nw.c++.html
 *        edited heavily by Melissa Gymrek
 -------------------------------------------*/

#include <sstream>
#include "nw.h"

using namespace std;
const int  a =  2;   // Match
const int  b = -2;   // Mismatch
const int  s[ 4 ][ 4 ] = { { a, b, b, b },  
			   { b, a, b, b },
			   { b, b, a, b },
			   { b, b, b, a } } ;
const int  d = 6 ;                 /* gap penalty */
const int GAPOPEN = 6;
const int GAPEXTEND = 0.1;

int nw(                                                          
       const string&       seq_1,          /*  Needleman-Wunsch   */
       const string&       seq_2,          /*  algorithm for      */
       string&      seq_1_al,       /*  global alignment   */
       string&      seq_2_al,       /*  of nt sequence.    */
       bool         prm,
       int* score,  /* SW alignment score */
       CIGAR_LIST* cigar_list
      )
{

  int  L1 = seq_1.length();
  int  L2 = seq_2.length();

  // Dynamic programming matrix
  vector<int > M((L1+1)*(L2+1));
  vector<int > I((L1+1)*(L2+1));

  // Traceback matrix
  std::vector<char> tracebackM((L1+1)*(L2+1));
  std::vector<char> tracebackI((L1+1)*(L2+1));
  
  // Initialize traceback and F matrix (fill in first row and column)
  dpm_init( &M, &tracebackM, L1, L2, 0 );
  dpm_init( &I, &tracebackI, L1, L2, d );

  // Create alignment
  nw_align_ag( &M, &I, &tracebackM, &tracebackI, seq_1, seq_2, seq_1_al, seq_2_al, d, score, cigar_list);
  
  return  0 ;
}


void  dpm_init( std::vector<int >* F, std::vector<char>* traceback, int L1, int L2, int d ) {
  F->at( 0) =  0 ;
  traceback->at(0) = 'n';
  int i=0, j=0;

  for( j = 1; j <= L1; j++ ) {
    F->at(0*(L1+1)+j)=  -j * d ;
    traceback->at(0*(L1+1)+j) = '-';
  }
  for( i = 1; i <= L2; i++ ) {
    F->at( i*(L1+1)+0) =  -i * d ;
    traceback->at(i*(L1+1)+0) = '|';
  }
}


int nw_align(std::vector<int > *    F,
	     std::vector<char>* traceback,
	     const string&     seq_1,
	     const string&     seq_2,
	     string&    seq_1_al,
	     string&    seq_2_al,
	     int        d,         // Gap penalty
	     int* score  // SW score
	     ){
  int        k = 0, x = 0, y = 0;
  int        fU, fD, fL ;
  char       ptr, nuc ;
  int        i = 0, j = 0;
  int  L1 = seq_1.length();
  int  L2 = seq_2.length();
  
  for( i = 1; i <= L2; i++ ) {
    for( j = 1; j <= L1; j++ )
      { 
	nuc = seq_1[ j-1 ] ;
	switch( nuc ) {
	case 'A':  x = 0 ;  break ;
	case 'C':  x = 1 ;  break ;
	case 'G':  x = 2 ;  break ;
	case 'T':  x = 3 ;
	}	       
	nuc = seq_2[ i-1 ] ;
	switch( nuc ) {
	case 'A':  y = 0 ;  break ;
	case 'C':  y = 1 ;  break ;
	case 'G':  y = 2 ;  break ;
	case 'T':  y = 3 ;
	}
	fU = F->at( (i-1)*(L1)+j)-d; // gap
	fD = F->at( (i-1)*(L1)+(j-1))+ s[ x ][ y ] ; // mismatch
	fL = F->at( (i)*(L1)+(j-1))-d; // gap
	
	F->at( i*(L1)+j) = max( fU, fD, fL, &ptr ) ;
	traceback->at(i*L1+j) = ptr;
      }
  }
  // get score
  i-- ; j-- ;
  *score = F->at( i*(L1)+j);
	
  while( i > 0 || j > 0 ) {
    switch( traceback->at(i*L1+j) ) {
    case '|' :      
      seq_1_al += '-' ; 
      seq_2_al += seq_2[ i-1 ] ; 
      i-- ;
      break ;

    case '\\':      
      seq_1_al += seq_1[ j-1 ] ; 
      seq_2_al += seq_2[ i-1 ] ; 
      i-- ;  j-- ;
      break ;

    case '-' :      
      seq_1_al += seq_1[ j-1 ] ; 
      seq_2_al += '-' ; 
      j-- ;
    }
    k++ ;
  }
  
  reverse( seq_1_al.begin(), seq_1_al.end() );
  reverse( seq_2_al.begin(), seq_2_al.end() );
  return  0 ;
}


int  max( int f1, int f2, int f3, char * ptr ) {
  int  max = 0 ;

  if( f1 >= f2 && f1 >= f3 ) {
    max = f1 ;
    *ptr = '|' ;
  }
  else if( f2 > f3 )              
    {
      max = f2 ;
      *ptr = '\\' ;
    }
  else
    {
      max = f3 ;
      *ptr = '-' ;
    }
  
  return  max ;   
}



int nw_align_ag(                  // Needleman-Wunsch algorithm with affine gap penalty
		std::vector<int > *    M,
		std::vector<int > *    I,
		std::vector<char>* tracebackM,
		std::vector<char>* tracebackI,
		const string&     seq_1,
		const string&     seq_2,
		string&    seq_1_al,
		string&    seq_2_al,
		int        d,         // Gap penalty
		int* score,  // SW score
		CIGAR_LIST* cigar_list // Cigar string
				  ) {
  int        k = 0, x = 0, y = 0;
  int        mD1,mD2,iU1,iU2,iL1,iL2;
  char       ptrM, ptrI, nuc ;
  int        i = 0, j = 0;
  int  L1 = seq_1.length();
  int  L2 = seq_2.length();
  string raw_cigar;
  for( i = 1; i <= L2; i++ ) {
    for( j = 1; j <= L1; j++ ) { 
      nuc = seq_1[ j-1 ] ;
      switch( nuc ) {
      case 'A':  x = 0 ;  break ;
      case 'C':  x = 1 ;  break ;
      case 'G':  x = 2 ;  break ;
      case 'T':  x = 3 ;
      }
		       
      nuc = seq_2[ i-1 ] ;
      switch( nuc ) {
      case 'A':  y = 0 ;  break ;
      case 'C':  y = 1 ;  break ;
      case 'G':  y = 2 ;  break ;
      case 'T':  y = 3 ;
      }

      mD1 = M->at((i-1)*(L1+1)+(j-1))+s[x][y]; // match from match matrix 
      mD2 = I->at((i-1)*(L1+1)+(j-1))+s[x][y]; // match from gap matrix
      M->at(i*(L1+1)+j) = maxM(mD1,mD2,&ptrM); // match score

      iU1 = M->at(i*(L1+1)+(j-1))-GAPOPEN;
      iU2 = I->at(i*(L1+1)+(j-1))-GAPEXTEND;
      iL1 = M->at((i-1)*(L1+1)+j) - GAPOPEN;
      iL2 = I->at((i-1)*(L1+1)+j) - GAPEXTEND;
      I->at(i*(L1+1)+j) = maxI(iU1,iU2,iL1,iL2,&ptrI); // gap score

      // set traceback matrix
      tracebackM->at(i*(L1+1)+j) = ptrM;
      tracebackI->at(i*(L1+1)+j) = ptrI;
    }
  }
  // get score
  i-- ; j-- ;
  bool inMatchMatrix = false;
  int maxRowScore = 0;
  int maxColScore = 0;
  bool MatchBestRow = false;
  bool GapBestRow = false;
  bool MatchBestCol = false;
  bool GapBestCol = false;
  int bestI = 0;
  int bestJ = 0;
  // get max M and max I right col
  while ( i > 0 ) {
    if (M->at(i*(L1+1)+L1) >= maxRowScore) {
      maxRowScore = M->at(i*(L1+1)+L1);
      MatchBestRow = true;
      GapBestRow = false;
      bestI = i;
    }
    if (I->at(i*(L1+1)+L1) >= maxRowScore) {
      maxRowScore = I->at(i*(L1+1)+L1);
      GapBestRow = true;
      MatchBestRow = false;
      bestI = i;
    }
    i--;
  }
  // get maxM and max I right col
  while (j > 0) {
    if (M->at(L2*(L1+1)+j) >= maxColScore) {
      maxColScore = M->at(L2*(L1+1)+j);
      MatchBestCol = true;
      GapBestCol = false;
      bestJ = j;
    }
    if (I->at(L2*(L1+1)+j) >= maxColScore) {
      maxColScore = I->at(L2*(L1+1)+j);
      GapBestCol = true;
      MatchBestCol = false;
      bestJ = j;
    }
    j--;
  }
  if (maxColScore > maxRowScore) {
    *score = maxColScore;
    i = L2;
    j = bestJ;
    if (MatchBestCol) {
      inMatchMatrix = true;
    }
  } else {
    i = bestI;
    j = L1;
    if (MatchBestRow) {
      inMatchMatrix = true;
    }
  }
  if (M->at(i*(L1+1)+j) >= I->at(i*(L1+1)+j)) {
    *score = M->at( i*(L1+1)+j);
    inMatchMatrix = true;
  } else {
    *score = I->at( i*(L1+1)+j);
    inMatchMatrix = false;
  }

  while( i > 0 || j > 0 ) {
    char switchchar = inMatchMatrix ? tracebackM->at(i*(L1+1)+j) : tracebackI->at(i*(L1+1)+j) ;
    //    cout << switchchar << endl;
    switch( switchchar ) {
    case 'a' :      
      seq_1_al += seq_1[j-1];
      seq_2_al += seq_2[ i-1 ] ; 
      i-- ; j--;
      inMatchMatrix = true;
      raw_cigar += "M";
      break ;

    case 'b':
      seq_1_al += seq_1[ j-1 ] ; 
      seq_2_al += seq_2[ i-1 ] ; 
      i-- ;  j-- ;
      inMatchMatrix = false;
      raw_cigar += "M";
      break ;
		  
    case 'c' :      
      seq_1_al += seq_1[ j-1 ] ; 
      seq_2_al += '-' ; 
      j-- ;
      inMatchMatrix = true;
      raw_cigar += "I";
      break;
		  
    case 'd':
      seq_1_al += seq_1[j-1];
      seq_2_al += '-';
      j-- ; 
      inMatchMatrix = false;
      raw_cigar += "I";
      break;
      
    case 'e':
      seq_1_al += "-";
      seq_2_al += seq_2[i-1];
      i-- ;
      inMatchMatrix = true;
      raw_cigar += "D";
      break;
      
    case 'f':
      seq_1_al += "-";
      seq_2_al += seq_2[i-1];
      i-- ;
      inMatchMatrix = false;
      raw_cigar += "D";
      break;

    case '-':
      seq_1_al += seq_1[j-1];
      seq_2_al += '-';
      j--;
      inMatchMatrix = true;
      raw_cigar += "I";
      break;
      
    case '|':
      seq_1_al += '-';
      seq_2_al += seq_2[i-1];
      i--;
      inMatchMatrix = false;
      raw_cigar += "D";
      break;
		 
    case 'n':
      i = 0; j = 0;
      break;
    }

    k++ ;
  }

  // simplify cigars core
  reverse(raw_cigar.begin(), raw_cigar.end() );
  char cigar_char = raw_cigar.at(0);
  char new_cigar_char;
  int num = 0;
  for (int i = 1; i < raw_cigar.length() ; i++) {
    new_cigar_char = raw_cigar.at(i);
    if (new_cigar_char != cigar_char) {
      CIGAR new_cigar;
      new_cigar.num = num+1;
      new_cigar.cigar_type = cigar_char;
      cigar_list->cigars.push_back(new_cigar);
      std::stringstream ss;
      ss << num+1 << cigar_char;
      cigar_list->cigar_string += ss.str();
      num = 0;
    } else {
      num+= 1;
    }
    cigar_char = new_cigar_char;
  }
  if (new_cigar_char == cigar_char) {
    std::stringstream ss;
    ss << num+1 << cigar_char;
    cigar_list->cigar_string += ss.str();
    CIGAR new_cigar;
    new_cigar.num = num+1;
    new_cigar.cigar_type = cigar_char;
    cigar_list->cigars.push_back(new_cigar);
  }
  reverse( seq_1_al.begin(), seq_1_al.end() );
  reverse( seq_2_al.begin(), seq_2_al.end() );
  return  0 ;
}



int  maxM( int f1, int f2, char * ptr )
{
        int  max = 0 ;

        if( f1 >= f2 )  
        {
                max = f1 ;
                *ptr = 'a' ;
        }
        else
        {
                max = f2 ;
                *ptr = 'b' ;
        }
        
        return  max ;   
}

int  maxI( int f1, int f2, int f3, int f4, char * ptr )
{
        int  max = 0 ;

        if( f1 >= f2 && f1 >= f3 && f1 >= f4) {
	  max = f1 ;
	  *ptr = 'c' ;
        } else if (f2 > f3 && f2 > f4) {
	  max = f2;
	  *ptr = 'd';
	} else if (f3 > f4) {
	  max = f3;
	  *ptr = 'e';
	} else {
	  max =f4;
	  *ptr = 'f';
	}
        return  max ;   
}
