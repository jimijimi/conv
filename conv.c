/*
  conv version 2.0 - command line unit conversion.
  Copyright (C) 2015 Jaime Ortiz
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#include <libgen.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>


#define VALID_NUM_ARGS 5
#define MIN_NUM_ARGS 2
#define MAX_CHARS 2048

struct List{ int n; char **l; };


struct Data{
	char *qty;
	char *from_unit;
	char *to_unit;
	float q;
};


struct Database{
	int n;
	char **from_unit;
	char **to_unit;
	char **factor;
	char **constant;
	char **exponent;
};


struct Result{
	float result;
        char valid;
};


void Help( );
void License( );
void ValidateCmd( int, const char ** );
void InitializeData( struct Data * );
void InitializeDatabase( struct Database * );
void InitializeResult( struct Result * );
void ResizeDatabase( struct Database * );
void PrintConv( struct Data *, struct Result * );
void ValidateData( struct Data * );
void CleanData( struct Data * );
void CleanList( struct List * );
void CleanDatabase( struct Database * );
void LoadDatabase( struct Database * );
void Convert( struct Database *, struct Data *, struct Result * );
struct List *Split( char *, char * );
char *Strip( char * );
void PrintList( struct List * );
void GetInstallationPath( char * );


int main( int argc, const char **argv )
{
	struct Data data;
	struct Database db;
	struct Result r;

	ValidateCmd( argc, argv );
	
	InitializeData( &data );
	InitializeDatabase( &db );
	InitializeResult( &r );

	data.qty = strdup( argv[1] );
	data.from_unit = strdup( argv[2] );
	data.to_unit = strdup( argv[4] );

	ValidateData( &data );

	LoadDatabase( &db );

	Convert( &db, &data, &r );

	PrintConv( &data, &r );

	CleanData( &data );

	CleanDatabase( &db );

	return 0;
}

void ValidateCmd( int argc, const char **argv )
{
	if ( argc == MIN_NUM_ARGS &&
	     ( strcmp( argv[1], "-l" ) || strcmp( argv[1], "--license" ) ) ){
		License();
		exit( 0 );
	}

	if ( argc > VALID_NUM_ARGS || argc < VALID_NUM_ARGS ){
		Help();
		exit( 1 );
	}

	if ( !strcmp( argv[3], "to" ) ||
	     !strcmp( argv[3], "TO" ) ||
	     !strcmp( argv[3], "To" ) ){
		;
	}
	else{
		Help();
		exit( 0 );
	}
}


void InitializeData( struct Data *d )
{
	d -> qty = NULL;
	d -> from_unit = NULL;
	d -> to_unit = NULL;
	d -> q = 0.0f;
}

void InitializeDatabase (struct Database *db )
{
	db -> n = 0.0f;
	db -> from_unit = NULL;
	db -> to_unit = NULL;
	db -> factor = NULL;
	db -> constant = NULL;
	db -> exponent = NULL;
}

void InitializeResult( struct Result *r )
{
	r -> result = 0.0f;
	r -> valid = 0;
}

void ResizeDatabase( struct Database *db )
{
	char **temp_array = NULL;

	db -> n += 1;

	temp_array = realloc( db -> from_unit, db -> n * sizeof( char** ) );
	db -> from_unit = temp_array;

	temp_array = realloc( db -> to_unit, db -> n * sizeof( char** ) );
	db -> to_unit = temp_array;

	temp_array = realloc( db -> factor, db -> n * sizeof( char** ) );
	db -> factor = temp_array;

	temp_array = realloc( db -> constant, db -> n * sizeof( char** ) );
	db -> constant = temp_array;

	temp_array = realloc( db -> exponent, db -> n * sizeof( char** ) );
	db -> exponent = temp_array;
}

void ValidateData( struct Data *d )
{
	int i = 0;
	float fprep = 0.0f;
	unsigned char valid = 1;

	for ( i = 0; i < strlen( d -> qty ); i ++ ){
		if( isalpha( d -> qty[ i ] ) ){
			valid = 0;
		}
	}

	fprep = atof( d -> qty );

	if ( fprep == 0 ){
		char strrep[ MAX_CHARS ];
#ifdef WINDOWS
		_snprintf( strrep, MAX_CHARS, "%f", fprep );
#else
		snprintf( strrep, MAX_CHARS, "%f", fprep );
#endif
		if( strcmp( strrep, d -> qty ) && valid ){
			printf( "The quantity provided is not valid\n" );
			printf( " you gave: %s", d -> qty );
			exit( 1 );
		}
	}

	d -> q = fprep;
}


void GetInstallationPath( char *path )
{
	char dir[ MAX_CHARS ];
	memset( dir, 0, MAX_CHARS );
#ifdef WINDOWS
	char full_path[ MAX_CHARS ];
	char drive[ MAX_CHARS ];
	_snprintf( full_path, MAX_CHARS, "%s", _pgmptr ); //%Fs
	_splitpath_s( full_path, drive, MAX_CHARS, dir, MAX_CHARS, NULL, 0, NULL, 0 );
	_snprintf( path, 2048, "%s%sconvdb.dat", drive, dir );
#else
	size_t len = readlink( "/proc/self/exe", dir, MAX_CHARS );
	snprintf( path, MAX_CHARS, "%sdb.dat", dir );
#endif
}


void LoadDatabase( struct Database *db )
{
	FILE *f = NULL;
	char line[ MAX_CHARS ];
	char final_path[ MAX_CHARS ];

	GetInstallationPath( final_path );
	
	f = fopen( final_path, "r" );

	if ( !f ){
		printf( "Databasefile convdb.dat does not exist.\n" );
		exit( 1 );
	}

	while( fgets( line, MAX_CHARS, f ) != NULL ){
		if ( strstr( line, "#" ) ){
			;
		}
		else if ( strlen( line ) > 7 ){
			struct List *temp_line = NULL;
			char *this_line = NULL;
			this_line = Strip( line );
			temp_line = Split( this_line, " " );
			if ( temp_line -> n < 5 ){
				printf( "Malformed database entry\n" );
				printf( "%s", line );
				printf( "Five (5) columns are needed:\n" );
				printf( "Original Units | Target Units | Factor | Constant | Exponent\n" );
				exit( 1 );
			}
			ResizeDatabase( db );
			db -> from_unit[ db -> n - 1 ] = strdup( temp_line -> l[0] );
			db -> to_unit[ db -> n - 1 ] = strdup( temp_line -> l[1] );
			db -> factor[ db -> n - 1 ] = strdup( temp_line -> l[2] );
			db -> constant[ db -> n - 1 ] = strdup( temp_line -> l[3] );
			db -> exponent[ db -> n - 1 ] = strdup( temp_line -> l[4] );
			CleanList( temp_line );
			free( this_line );
		}
	}
	fclose( f );
}


void Convert( struct Database *db, struct Data *d, struct Result *r )
{
	unsigned int i = 0;
	for( i = 0; i < db -> n; i++ ){
		if( !strcmp( db -> from_unit[i], d -> from_unit ) &&
		    !strcmp( db -> to_unit[i], d -> to_unit ) ){
			r -> result = pow( atof( d -> qty ), atof( db ->exponent[ i ] ) ) * atof( db -> factor[ i ] ) +
				atof( db -> constant[ i ] );
			r -> valid = 1;
			return;
		}
	}
}


void PrintList( struct List *l )
{
	unsigned int i = 0;
	for( i = 0; i < l -> n; i++ ){
		printf( "%s", l -> l[i] );
	}
	printf("\n");
}


void PrintConv( struct Data *d, struct Result *r )
{
	if ( r -> valid ){
		printf( "%.4f %s = %f %s\n", d -> q, d -> from_unit, r -> result, d -> to_unit );
	}
	else{
		printf( "Cannot convert from %s to %s.\n", d -> from_unit, d -> to_unit );
		printf( "The units are not in the database.\n" );
	}
}

void CleanData( struct Data *d )
{
	free( d -> qty );
	free( d -> from_unit );
	free( d -> to_unit );
}

void CleanList( struct List *list )
{
	int i = 0;
	for( i = 0; i < list -> n; i++ ){
		free( list -> l[ i ] );
	}
	free( list -> l );
	free( list );
}

void CleanDatabase( struct Database *db )
{
	int i = 0;
	for ( i = 0; i < db -> n; i ++ ){
		free( db -> from_unit[ i ] );
		free( db -> to_unit[ i ] );
		free( db -> factor[ i ] );
		free( db -> constant[ i ] );
		free( db -> exponent[ i ] );
	}
	free( db -> from_unit );
	free( db -> to_unit );
	free( db -> factor );
	free( db -> constant );
	free( db -> exponent );
}

char *Strip( char *s )
{
	char *dup_s = strdup( s );
	unsigned int len_dup_s = strlen( dup_s );

	char *new_s = malloc( ( len_dup_s + 2 ) * sizeof( char ) );
	unsigned int i = 0;

	for ( i = 0; i < strlen( dup_s ); i ++ ){
		if ( dup_s[ i ] == '\n' ){
			new_s[ i ] = '\0';
			break;
		}
		new_s[ i ] = dup_s[ i ];
	}

	free( dup_s );
	return new_s;
}

struct List *Split( char *s_a, char *del )
{
	char *pw = NULL;
	char *temp_s_a = NULL;
	char **temp_returned_list = NULL;
	int word_len = 0;

	struct List *final_list = NULL;
	final_list = malloc( sizeof( struct List ) );
	final_list -> l = NULL;
	final_list -> n = 0;

	temp_s_a = strdup( s_a );
	pw = strtok( temp_s_a, del );
	while( pw != NULL ){
		temp_returned_list = realloc( final_list -> l,
					      ( final_list -> n + 1 ) * sizeof( char ** ) );
		final_list -> l = temp_returned_list;
		word_len = strlen( pw );
		final_list -> l[ final_list -> n ] = strdup( pw );
		pw = strtok( NULL, del );
		final_list -> n += 1;
	}
	free( temp_s_a );
	return final_list;
}

void Help()
{
	printf( "conv version 1.0 - command line unit conversion,\n" 
		"Copyright (C) 2015 Jaime Ortiz\n"
		"conv comes with ABSOLUTELY NO WARRANTY; for details type `conv -l'.\n"
		"This is free software, and you are welcome to redistribute it\n"
		"under certain conditions; type `conv --license' for details.\n\n" );
	printf( "USAGE:\n" );
	printf( "  conv [ QTY ] [ FROM_UNIT ] TO [ TO_UNIT ] <enter>\n\n"
		"  QTY      : numerical value integer or decimal\n"
		"  FROM_UNIT: numerical value, i.e. kg, m, Km, m/s, etc.\n"
		"  TO/to    : this word is mandatory. This makes the command\n"
		"             more natural\n"
		"  TO_UNIT  : target units. i.e. kg, m, km, m/s, etc.\n\n"
		"  example:\n"
		"       $ conv 2 m to km <enter>\n\n"
		"  output:\n"
		"       $ 2.0000 m = 0.002000 km\n\n"
		"LICENSE INFO:\n"
		"  -l --license\n\n"
		"  conv is 2015 (c) Jaime Ortiz\n\n"
		"CONTACT:\n"
		"  quark.charm at g mail dot com\n" );
}

void License( )
{
	printf( "               GNU GENERAL PUBLIC LICENSE\n"
		"                  Version 2, June 1991\n\n"

		"Copyright (C) 1989, 1991 Free Software Foundation, Inc.,\n"
		"51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n"
		"Everyone is permitted to copy and distribute verbatim copies\n"
		"of this license document, but changing it is not allowed.\n\n"
		
		"Preamble\n\n"
	  
		"The licenses for most software are designed to take away your\n"
		"freedom to share and change it.  By contrast, the GNU General Public\n"
		"License is intended to guarantee your freedom to share and change free\n"
		"software--to make sure the software is free for all its users.  This\n"
		"General Public License applies to most of the Free Software\n"
		"Foundation's software and to any other program whose authors commit to\n"
		"using it.  (Some other Free Software Foundation software is covered by\n"
		"the GNU Lesser General Public License instead.)  You can apply it to\n"
		"your programs, too.\n\n"

		"  When we speak of free software, we are referring to freedom, not\n"
		"price.  Our General Public Licenses are designed to make sure that you\n"
		"have the freedom to distribute copies of free software (and charge for\n"
		"this service if you wish), that you receive source code or can get it\n"
		"if you want it, that you can change the software or use pieces of it\n"
		"in new free programs; and that you know you can do these things.\n\n"

		"  To protect your rights, we need to make restrictions that forbid\n"
		"anyone to deny you these rights or to ask you to surrender the rights.\n"
		"These restrictions translate to certain responsibilities for you if you\n"
		"distribute copies of the software, or if you modify it.\n\n"
		
		"  For example, if you distribute copies of such a program, whether\n"
		"gratis or for a fee, you must give the recipients all the rights that\n"
		"you have.  You must make sure that they, too, receive or can get the\n"
		"source code.  And you must show them these terms so they know their\n"
		"rights.\n\n"
		
		"  We protect your rights with two steps: (1) copyright the software, and\n"
		"(2) offer you this license which gives you legal permission to copy,\n"
		"distribute and/or modify the software.\n\n"

		"  Also, for each author's protection and ours, we want to make certain\n"
		"that everyone understands that there is no warranty for this free\n"
		"software.  If the software is modified by someone else and passed on, we\n"
		"want its recipients to know that what they have is not the original, so\n"
		"that any problems introduced by others will not reflect on the original\n"
		"authors' reputations.\n\n"
		
		"  Finally, any free program is threatened constantly by software\n"
		"patents.  We wish to avoid the danger that redistributors of a free\n"
		"program will individually obtain patent licenses, in effect making the\n"
		"program proprietary.  To prevent this, we have made it clear that any\n"
		"patent must be licensed for everyone's free use or not licensed at all.\n\n"
		
		"  The precise terms and conditions for copying, distribution and\n"
		"modification follow.\n\n"
		
		"                    GNU GENERAL PUBLIC LICENSE\n"
		"   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n\n"

		"  0. This License applies to any program or other work which contains\n"
		"a notice placed by the copyright holder saying it may be distributed\n"
		"under the terms of this General Public License.  The \"Program\", below,\n"
		"refers to any such program or work, and a \"work based on the Program\"\n"
		"means either the Program or any derivative work under copyright law:\n"
		"that is to say, a work containing the Program or a portion of it,\n"
		"either verbatim or with modifications and/or translated into another\n"
		"language.  (Hereinafter, translation is included without limitation in\n"
		"the term \"modification\".)  Each licensee is addressed as \"you\".\n\n"
		
		"Activities other than copying, distribution and modification are not\n"
		"covered by this License; they are outside its scope.  The act of\n"
		"running the Program is not restricted, and the output from the Program\n"
		"is covered only if its contents constitute a work based on the\n"
		"Program (independent of having been made by running the Program).\n"
		"Whether that is true depends on what the Program does.\n\n"
		
		"  1. You may copy and distribute verbatim copies of the Program's\n"
		"source code as you receive it, in any medium, provided that you\n"
		"conspicuously and appropriately publish on each copy an appropriate\n"
		"copyright notice and disclaimer of warranty; keep intact all the\n"
		"notices that refer to this License and to the absence of any warranty;\n"
		"and give any other recipients of the Program a copy of this License\n"
		"along with the Program.\n\n"

		"You may charge a fee for the physical act of transferring a copy, and\n"
		"you may at your option offer warranty protection in exchange for a fee.\n\n"
		
		"  2. You may modify your copy or copies of the Program or any portion\n"
		"of it, thus forming a work based on the Program, and copy and\n"
		"distribute such modifications or work under the terms of Section 1\n"
		"above, provided that you also meet all of these conditions:\n\n"
		
		"    a) You must cause the modified files to carry prominent notices\n"
		"    stating that you changed the files and the date of any change.\n\n"
		
		"    b) You must cause any work that you distribute or publish, that in\n"
		"    whole or in part contains or is derived from the Program or any\n"
		"    part thereof, to be licensed as a whole at no charge to all third\n"
		"    parties under the terms of this License.\n\n"
		
		"    c) If the modified program normally reads commands interactively\n"
		"    when run, you must cause it, when started running for such\n"
		"    interactive use in the most ordinary way, to print or display an\n"
		"    announcement including an appropriate copyright notice and a\n"
		"    notice that there is no warranty (or else, saying that you provide\n"
		"    a warranty) and that users may redistribute the program under\n"
		"    these conditions, and telling the user how to view a copy of this\n"
		"    License.  (Exception: if the Program itself is interactive but\n"
		"    does not normally print such an announcement, your work based on\n"
		"    the Program is not required to print an announcement.)\n\n"
		
		"These requirements apply to the modified work as a whole.  If\n"
		"identifiable sections of that work are not derived from the Program,\n"
		"and can be reasonably considered independent and separate works in\n"
		"themselves, then this License, and its terms, do not apply to those\n"
		"sections when you distribute them as separate works.  But when you\n"
		"distribute the same sections as part of a whole which is a work based\n"
		"on the Program, the distribution of the whole must be on the terms of\n"
		"this License, whose permissions for other licensees extend to the\n"
		"entire whole, and thus to each and every part regardless of who wrote it.\n\n"
		
		"Thus, it is not the intent of this section to claim rights or contest\n"
		"your rights to work written entirely by you; rather, the intent is to\n"
		"exercise the right to control the distribution of derivative or\n"
		"collective works based on the Program.\n\n"
		
		"In addition, mere aggregation of another work not based on the Program\n"
		"with the Program (or with a work based on the Program) on a volume of\n"
		"a storage or distribution medium does not bring the other work under\n"
		"the scope of this License.\n\n"

		"  3. You may copy and distribute the Program (or a work based on it,\n"
		"under Section 2) in object code or executable form under the terms of\n"
		"Sections 1 and 2 above provided that you also do one of the following:\n\n"
		
		"    a) Accompany it with the complete corresponding machine-readable\n"
		"    source code, which must be distributed under the terms of Sections\n"
		"    1 and 2 above on a medium customarily used for software interchange; or,\n\n"
		
		"    b) Accompany it with a written offer, valid for at least three\n"
		"    years, to give any third party, for a charge no more than your\n"
		"    cost of physically performing source distribution, a complete\n"
		"    machine-readable copy of the corresponding source code, to be\n"
		"    distributed under the terms of Sections 1 and 2 above on a medium\n"
		"    customarily used for software interchange; or,\n\n"
		
		"    c) Accompany it with the information you received as to the offer\n"
		"    to distribute corresponding source code.  (This alternative is\n"
		"    allowed only for noncommercial distribution and only if you\n"
		"    received the program in object code or executable form with such\n"
		"    an offer, in accord with Subsection b above.)\n\n"
		
		"The source code for a work means the preferred form of the work for\n"
		"making modifications to it.  For an executable work, complete source\n"
		"code means all the source code for all modules it contains, plus any\n"
		"associated interface definition files, plus the scripts used to\n"
		"control compilation and installation of the executable.  However, as a\n"
		"special exception, the source code distributed need not include\n"
		"anything that is normally distributed (in either source or binary\n"
		"form) with the major components (compiler, kernel, and so on) of the\n"
		"operating system on which the executable runs, unless that component\n"
		"itself accompanies the executable.\n\n"

		"If distribution of executable or object code is made by offering\n"
		"access to copy from a designated place, then offering equivalent\n"
		"access to copy the source code from the same place counts as\n"
		"distribution of the source code, even though third parties are not\n"
		"compelled to copy the source along with the object code.\n\n"

		"  4. You may not copy, modify, sublicense, or distribute the Program\n"
		"except as expressly provided under this License.  Any attempt\n"
		"otherwise to copy, modify, sublicense or distribute the Program is\n"
		"void, and will automatically terminate your rights under this License.\n"
		"However, parties who have received copies, or rights, from you under\n"
		"this License will not have their licenses terminated so long as such\n"
		"parties remain in full compliance.\n\n"

		"  5. You are not required to accept this License, since you have not\n"
		"signed it.  However, nothing else grants you permission to modify or\n"
		"distribute the Program or its derivative works.  These actions are\n"
		"prohibited by law if you do not accept this License.  Therefore, by\n"
		"modifying or distributing the Program (or any work based on the\n"
		"Program), you indicate your acceptance of this License to do so, and\n"
		"all its terms and conditions for copying, distributing or modifying\n"
		"the Program or works based on it.\n\n"

		"  6. Each time you redistribute the Program (or any work based on the\n"
		"Program), the recipient automatically receives a license from the\n"
		"original licensor to copy, distribute or modify the Program subject to\n"
		"these terms and conditions.  You may not impose any further\n"
		"restrictions on the recipients' exercise of the rights granted herein.\n"
		"You are not responsible for enforcing compliance by third parties to\n"
		"this License.\n\n"

		"  7. If, as a consequence of a court judgment or allegation of patent\n"
		"infringement or for any other reason (not limited to patent issues),\n"
		"conditions are imposed on you (whether by court order, agreement or\n"
		"otherwise) that contradict the conditions of this License, they do not\n"
		"excuse you from the conditions of this License.  If you cannot\n"
		"distribute so as to satisfy simultaneously your obligations under this\n"
		"License and any other pertinent obligations, then as a consequence you\n"
		"may not distribute the Program at all.  For example, if a patent\n"
		"license would not permit royalty-free redistribution of the Program by\n"
		"all those who receive copies directly or indirectly through you, then\n"
		"the only way you could satisfy both it and this License would be to\n"
		"refrain entirely from distribution of the Program.\n\n"
		
		"If any portion of this section is held invalid or unenforceable under\n"
		"any particular circumstance, the balance of the section is intended to\n"
		"apply and the section as a whole is intended to apply in other\n"
		"circumstances.\n\n"
		
		"It is not the purpose of this section to induce you to infringe any\n"
		"patents or other property right claims or to contest validity of any\n"
		"such claims; this section has the sole purpose of protecting the\n"
		"integrity of the free software distribution system, which is\n"
		"implemented by public license practices.  Many people have made\n"
		"generous contributions to the wide range of software distributed\n"
		"through that system in reliance on consistent application of that\n"
		"system; it is up to the author/donor to decide if he or she is willing\n"
		"to distribute software through any other system and a licensee cannot\n"
		"impose that choice.\n\n"
		
		"This section is intended to make thoroughly clear what is believed to\n"
		"be a consequence of the rest of this License.\n\n"

		"  8. If the distribution and/or use of the Program is restricted in\n"
		"certain countries either by patents or by copyrighted interfaces, the\n"
		"original copyright holder who places the Program under this License\n"
		"may add an explicit geographical distribution limitation excluding\n"
		"those countries, so that distribution is permitted only in or among\n"
		"countries not thus excluded.  In such case, this License incorporates\n"
		"the limitation as if written in the body of this License.\n\n"

		"  9. The Free Software Foundation may publish revised and/or new versions\n"
		"of the General Public License from time to time.  Such new versions will\n"
		"be similar in spirit to the present version, but may differ in detail to\n"
		"address new problems or concerns.\n\n"
		
		"Each version is given a distinguishing version number.  If the Program\n"
		"specifies a version number of this License which applies to it and \"any\n"
		"later version\", you have the option of following the terms and conditions\n"
		"either of that version or of any later version published by the Free\n"
		"Software Foundation.  If the Program does not specify a version number of\n"
		"this License, you may choose any version ever published by the Free Software\n"
		"Foundation.\n\n"
		
		"  10. If you wish to incorporate parts of the Program into other free\n"
		"programs whose distribution conditions are different, write to the author\n"
		"to ask for permission.  For software which is copyrighted by the Free\n"
		"Software Foundation, write to the Free Software Foundation; we sometimes\n"
		"make exceptions for this.  Our decision will be guided by the two goals\n"
		"of preserving the free status of all derivatives of our free software and\n"
		"of promoting the sharing and reuse of software generally.\n\n"
		
		"                            NO WARRANTY\n"

		"  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n"
		"FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n"
		"OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n"
		"PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n"
		"OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"
		"MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n"
		"TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n"
		"PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n"
		"REPAIR OR CORRECTION.\n\n"
		
		"  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n"
		"WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n"
		"REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n"
		"INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n"
		"OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n"
		"TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n"
		"YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n"
		"PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n"
		"POSSIBILITY OF SUCH DAMAGES.\n\n"

		"                     END OF TERMS AND CONDITIONS\n" 
		"	Appendix: How to Apply These Terms to Your New Programs\n"

		"  If you develop a new program, and you want it to be of the greatest\n"
		"possible use to the public, the best way to achieve this is to make it\n"
		"free software which everyone can redistribute and change under these terms.\n"

		"  To do so, attach the following notices to the program.  It is safest\n"
		"to attach them to the start of each source file to most effectively\n"
		"convey the exclusion of warranty; and each file should have at least\n"
		"the \"copyright\" line and a pointer to where the full notice is found.\n"

		"    <one line to give the program's name and a brief idea of what it does.>\n"
		"    Copyright (C) 19yy  <name of author>\n"

		"    This program is free software; you can redistribute it and/or modify\n"
		"    it under the terms of the GNU General Public License as published by\n"
		"    the Free Software Foundation; either version 2 of the License, or\n"
		"    (at your option) any later version.\n"

		"    This program is distributed in the hope that it will be useful,\n"
		"    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"    GNU General Public License for more details.\n"

		"    You should have received a copy of the GNU General Public License\n"
		"    along with this program; if not, write to the Free Software\n"
		"    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n"

		"Also add information on how to contact you by electronic and paper mail.\n"

		"If the program is interactive, make it output a short notice like this\n"
		"when it starts in an interactive mode:\n"

		"    Gnomovision version 69, Copyright (C) 19yy name of author\n"
		"    Gnomovision comes with ABSOLUTELY NO WARRANTY; for details type `show w'.\n"
		"    This is free software, and you are welcome to redistribute it\n"
		"    under certain conditions; type `show c' for details.\n"

		"The hypothetical commands `show w' and `show c' should show the appropriate\n"
		"parts of the General Public License.  Of course, the commands you use may\n"
		"be called something other than `show w' and `show c'; they could even be\n"
		"mouse-clicks or menu items--whatever suits your program.\n"

		"You should also get your employer (if you work as a programmer) or your\n"
		"school, if any, to sign a \"copyright disclaimer\" for the program, if\n"
		"necessary.  Here is a sample; alter the names:\n"

		"  Yoyodyne, Inc., hereby disclaims all copyright interest in the program\n"
		"  `Gnomovision' (which makes passes at compilers) written by James Hacker.\n"

		"  <signature of Ty Coon>, 1 April 1989\n"
		"  Ty Coon, President of Vice\n"

		"This General Public License does not permit incorporating your program into\n"
		"proprietary programs.  If your program is a subroutine library, you may\n"
		"consider it more useful to permit linking proprietary applications with the\n"
		"library.  If this is what you want to do, use the GNU Library General\n"
		"Public License instead of this License.\n" );
}
