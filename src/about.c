/* Bluefish HTML Editor
 * about.c
 *
 *Copyright (C) 2004 Eugene Morenko(More) more@irpin.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* getopt() */

#include "about.h"
#include "bluefish.h"
#include "gtk_easy.h"

#define BORDER 6

static const gchar *INFO= "\n\
Bluefish is a powerful editor for experienced web designers and programmers.\n\
Bluefish supports many programming and markup languages, \n\
but it focuses on editing dynamic and interactive websites.\n\n\
Bluefish is an open source development project, released under the GPL license.\n\
\n\
Bluefish runs on most (all?) POSIX compatible operating systems including Linux,\n\
FreeBSD, MacOS-X, OpenBSD, Solaris and Tru64.\n\n\
Bluefish Website\n\
http://bluefish.openoffice.nl\n\
";

static const gchar *AUTHORS= "\n\
Project lead\n\
  Olivier Sessink <olivier@bluefish.openoffice.nl>\n\
\n\
Core developers\n\
  Jim Hayward <jimhayward@linuxexperience.com>\n\
  Oskar Świda <swida@aragorn.pb.bialystok.pl>\n\
  Christian Tellefsen <christian@tellefsen.net>\n\
  Eugene Morenko(More) <more@irpin.com>\n\
\n\
Documentation:\n\
  Alastair Porter <alastair@linuxexperience.com>\n\
  Daniel Blair <joecamel@realcoders.org>\n\
\n\
Package Maintainers:\n\
  Debain:   Davide Puricelli <evo@debian.org>\n\
  Redhat:   Matthias Haase <matthias_haase@bennewitz.com>\n\
  Mandrake: Todd Lyons <todd@mrball.net>\n\
\n\
If you know of anyone missing from this list, please let us know on\n\
<bluefish-dev@lists.ems.ru>\n\
\n\
Thanks to all who helped making this software available.\n\
";

static const gchar *TRANSLATORS = "\n\
Translators:\n\
  Danish (DA) -  Rasmus Toftdahl Olesen <rto@pohldata.dk>\n\
  French (FR) -  Roméo Viu-Berges <apostledemencia@free.fr>\n\
  German (DE) -  Roland Steinbach <roland@netzblick.de>\n\
  Hungarian (HU) -  Péter Sáska <sasek@ccsystem.hu>\n\
  Italian (IT) -  Stefano Canepa <sc@linux.it>\n\
  Polish (PL) -  Oskar Swida <swida@aragorn.pb.bialystok.pl>\n\
  Portuguese (PT_BR) -  Anderson Rocha <anderson@maxlinux.com.br>\n\
  Russian (RU) -  Eugene Rupakov <rupakov@jet.msk.ru>\n\
  Spanish (ES) -  Walter Oscar Echarri <wecharri@infovia.com.ar>\n\
  Swedish (SV) -  David Smeringe <david.smeringe@telia.com>\n\
  Chinese (zh_CN) -  Ting Yang (Dormouse) <mouselinux@163.com>\n\
  Finnish (FI) -  Juho Roukala <j.mr@luukku.com>\n\
\n\
Outdated translations (status 04-01-2004):\n\
  Bulgarian (BG) -  Peio Popov <peio@peio.org>\n\
  Norwegian (NO) -  Christian Tellefsen <chris@tellefsen.net>\n\
  Japanese (JA) -  Ryoichi INAGAKI <ryo1@bc.wakwak.com>\n\
  Catalan (CA) -  Jordi Sánchez <parki@softhome.net>\n\
  Dutch (NL) -  Jan Bok Diemen <lunixbox@xs4all.nl>\n\
  Greek (EL) -  Constantin Fabrikezis <phi-kappa@tiscali.fr>\n\
";

static const char *LICENSE = "GNU GENERAL PUBLIC LICENSE\n\
Version 2, June 1991\n\
\n\
Copyright (C) 1989, 1991 Free Software Foundation, Inc.\n\
59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n\
Everyone is permitted to copy and distribute verbatim copies\n\
of this license document, but changing it is not allowed.\n\
\n\
Preamble\n\
\n\
The licenses for most software are designed to take away your\n\
freedom to share and change it.  By contrast, the GNU General Public\n\
License is intended to guarantee your freedom to share and change free\n\
software--to make sure the software is free for all its users.  This\n\
General Public License applies to most of the Free Software\n\
Foundation's software and to any other program whose authors commit to\n\
using it.  (Some other Free Software Foundation software is covered by\n\
the GNU Library General Public License instead.)  You can apply it to\n\
your programs, too.\n\
\n\
When we speak of free software, we are referring to freedom, not\n\
price.  Our General Public Licenses are designed to make sure that you\n\
have the freedom to distribute copies of free software (and charge for\n\
this service if you wish), that you receive source code or can get it\n\
if you want it, that you can change the software or use pieces of it\n\
in new free programs; and that you know you can do these things.\n\
\n\
To protect your rights, we need to make restrictions that forbid\n\
anyone to deny you these rights or to ask you to surrender the rights.\n\
These restrictions translate to certain responsibilities for you if you\n\
distribute copies of the software, or if you modify it.\n\
\n\
For example, if you distribute copies of such a program, whether\n\
gratis or for a fee, you must give the recipients all the rights that\n\
you have.  You must make sure that they, too, receive or can get the\n\
source code.  And you must show them these terms so they know their\n\
rights.\n\
\n\
We protect your rights with two steps: (1) copyright the software, and\n\
(2) offer you this license which gives you legal permission to copy,\n\
distribute and/or modify the software.\n\
\n\
Also, for each author's protection and ours, we want to make certain\n\
that everyone understands that there is no warranty for this free\n\
software.  If the software is modified by someone else and passed on, we\n\
want its recipients to know that what they have is not the original, so\n\
that any problems introduced by others will not reflect on the original\n\
authors's reputations.\n\
\n\
Finally, any free program is threatened constantly by software\n\
patents.  We wish to avoid the danger that redistributors of a free\n\
program will individually obtain patent licenses, in effect making the\n\
program proprietary.  To prevent this, we have made it clear that any\n\
patent must be licensed for everyone's free use or not licensed at all.\n\
\n\
The precise terms and conditions for copying, distribution and\n\
modification follow.\n\
\n\
GNU GENERAL PUBLIC LICENSE\n\
TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n\
\n\
0. This License applies to any program or other work which contains\n\
a notice placed by the copyright holder saying it may be distributed\n\
under the terms of this General Public License.  The \"Program\", below,\n\
refers to any such program or work, and a \"work based on the Program\"\n\
means either the Program or any derivative work under copyright law:\n\
that is to say, a work containing the Program or a portion of it,\n\
either verbatim or with modifications and/or translated into another\n\
language.  (Hereinafter, translation is included without limitation in\n\
the term \"modification\".)  Each licensee is addressed as \"you\".\n\
\n\
Activities other than copying, distribution and modification are not\n\
covered by this License; they are outside its scope.  The act of\n\
running the Program is not restricted, and the output from the Program\n\
is covered only if its contents constitute a work based on the\n\
Program (independent of having been made by running the Program).\n\
Whether that is true depends on what the Program does.\n\
\n\
1. You may copy and distribute verbatim copies of the Program's\n\
source code as you receive it, in any medium, provided that you\n\
conspicuously and appropriately publish on each copy an appropriate\n\
copyright notice and disclaimer of warranty; keep intact all the\n\
notices that refer to this License and to the absence of any warranty;\n\
and give any other recipients of the Program a copy of this License\n\
along with the Program.\n\
\n\
You may charge a fee for the physical act of transferring a copy, and\n\
you may at your option offer warranty protection in exchange for a fee.\n\
\n\
2. You may modify your copy or copies of the Program or any portion\n\
of it, thus forming a work based on the Program, and copy and\n\
distribute such modifications or work under the terms of Section 1\n\
above, provided that you also meet all of these conditions:\n\
\n\
    a) You must cause the modified files to carry prominent notices\n\
    stating that you changed the files and the date of any change.\n\
\n\
    b) You must cause any work that you distribute or publish, that in\n\
    whole or in part contains or is derived from the Program or any\n\
    part thereof, to be licensed as a whole at no charge to all third\n\
    parties under the terms of this License.\n\
\n\
    c) If the modified program normally reads commands interactively\n\
    when run, you must cause it, when started running for such\n\
    interactive use in the most ordinary way, to print or display an\n\
    announcement including an appropriate copyright notice and a\n\
    notice that there is no warranty (or else, saying that you provide\n\
    a warranty) and that users may redistribute the program under\n\
    these conditions, and telling the user how to view a copy of this\n\
    License.  (Exception: if the Program itself is interactive but\n\
    does not normally print such an announcement, your work based on\n\
    the Program is not required to print an announcement.)\n\
\n\
These requirements apply to the modified work as a whole.  If\n\
identifiable sections of that work are not derived from the Program,\n\
and can be reasonably considered independent and separate works in\n\
themselves, then this License, and its terms, do not apply to those\n\
sections when you distribute them as separate works.  But when you\n\
distribute the same sections as part of a whole which is a work based\n\
on the Program, the distribution of the whole must be on the terms of\n\
this License, whose permissions for other licensees extend to the\n\
entire whole, and thus to each and every part regardless of who wrote it.\n\
\n\
Thus, it is not the intent of this section to claim rights or contest\n\
your rights to work written entirely by you; rather, the intent is to\n\
exercise the right to control the distribution of derivative or\n\
collective works based on the Program.\n\
\n\
In addition, mere aggregation of another work not based on the Program\n\
with the Program (or with a work based on the Program) on a volume of\n\
a storage or distribution medium does not bring the other work under\n\
the scope of this License.\n\
\n\
3. You may copy and distribute the Program (or a work based on it,\n\
under Section 2) in object code or executable form under the terms of\n\
Sections 1 and 2 above provided that you also do one of the following:\n\
\n\
    a) Accompany it with the complete corresponding machine-readable\n\
    source code, which must be distributed under the terms of Sections\n\
    1 and 2 above on a medium customarily used for software interchange; or,\n\
\n\
    b) Accompany it with a written offer, valid for at least three\n\
    years, to give any third party, for a charge no more than your\n\
    cost of physically performing source distribution, a complete\n\
    machine-readable copy of the corresponding source code, to be\n\
    distributed under the terms of Sections 1 and 2 above on a medium\n\
    customarily used for software interchange; or,\n\
\n\
    c) Accompany it with the information you received as to the offer\n\
    to distribute corresponding source code.  (This alternative is\n\
    allowed only for noncommercial distribution and only if you\n\
    received the program in object code or executable form with such\n\
    an offer, in accord with Subsection b above.)\n\
\n\
The source code for a work means the preferred form of the work for\n\
making modifications to it.  For an executable work, complete source\n\
code means all the source code for all modules it contains, plus any\n\
associated interface definition files, plus the scripts used to\n\
control compilation and installation of the executable.  However, as a\n\
special exception, the source code distributed need not include\n\
anything that is normally distributed (in either source or binary\n\
form) with the major components (compiler, kernel, and so on) of the\n\
operating system on which the executable runs, unless that component\n\
itself accompanies the executable.\n\
\n\
If distribution of executable or object code is made by offering\n\
access to copy from a designated place, then offering equivalent\n\
access to copy the source code from the same place counts as\n\
distribution of the source code, even though third parties are not\n\
compelled to copy the source along with the object code.\n\
\n\
4. You may not copy, modify, sublicense, or distribute the Program\n\
except as expressly provided under this License.  Any attempt\n\
otherwise to copy, modify, sublicense or distribute the Program is\n\
void, and will automatically terminate your rights under this License.\n\
However, parties who have received copies, or rights, from you under\n\
this License will not have their licenses terminated so long as such\n\
parties remain in full compliance.\n\
\n\
5. You are not required to accept this License, since you have not\n\
signed it.  However, nothing else grants you permission to modify or\n\
distribute the Program or its derivative works.  These actions are\n\
prohibited by law if you do not accept this License.  Therefore, by\n\
modifying or distributing the Program (or any work based on the\n\
Program), you indicate your acceptance of this License to do so, and\n\
all its terms and conditions for copying, distributing or modifying\n\
the Program or works based on it.\n\
\n\
6. Each time you redistribute the Program (or any work based on the\n\
Program), the recipient automatically receives a license from the\n\
original licensor to copy, distribute or modify the Program subject to\n\
these terms and conditions.  You may not impose any further\n\
restrictions on the recipients' exercise of the rights granted herein.\n\
You are not responsible for enforcing compliance by third parties to\n\
this License.\n\
\n\
7. If, as a consequence of a court judgment or allegation of patent\n\
infringement or for any other reason (not limited to patent issues),\n\
conditions are imposed on you (whether by court order, agreement or\n\
otherwise) that contradict the conditions of this License, they do not\n\
excuse you from the conditions of this License.  If you cannot\n\
distribute so as to satisfy simultaneously your obligations under this\n\
License and any other pertinent obligations, then as a consequence you\n\
may not distribute the Program at all.  For example, if a patent\n\
license would not permit royalty-free redistribution of the Program by\n\
all those who receive copies directly or indirectly through you, then\n\
the only way you could satisfy both it and this License would be to\n\
refrain entirely from distribution of the Program.\n\
\n\
If any portion of this section is held invalid or unenforceable under\n\
any particular circumstance, the balance of the section is intended to\n\
apply and the section as a whole is intended to apply in other\n\
circumstances.\n\
\n\
It is not the purpose of this section to induce you to infringe any\n\
patents or other property right claims or to contest validity of any\n\
such claims; this section has the sole purpose of protecting the\n\
integrity of the free software distribution system, which is\n\
implemented by public license practices.  Many people have made\n\
generous contributions to the wide range of software distributed\n\
through that system in reliance on consistent application of that\n\
system; it is up to the author/donor to decide if he or she is willing\n\
to distribute software through any other system and a licensee cannot\n\
impose that choice.\n\
\n\
This section is intended to make thoroughly clear what is believed to\n\
be a consequence of the rest of this License.\n\
\n\
8. If the distribution and/or use of the Program is restricted in\n\
certain countries either by patents or by copyrighted interfaces, the\n\
original copyright holder who places the Program under this License\n\
may add an explicit geographical distribution limitation excluding\n\
those countries, so that distribution is permitted only in or among\n\
countries not thus excluded.  In such case, this License incorporates\n\
the limitation as if written in the body of this License.\n\
\n\
9. The Free Software Foundation may publish revised and/or new versions\n\
of the General Public License from time to time.  Such new versions will\n\
be similar in spirit to the present version, but may differ in detail to\n\
address new problems or concerns.\n\
 \n\
Each version is given a distinguishing version number.  If the Program\n\
specifies a version number of this License which applies to it and \"any\n\
later version\", you have the option of following the terms and conditions\n\
either of that version or of any later version published by the Free\n\
Software Foundation.  If the Program does not specify a version number of\n\
this License, you may choose any version ever published by the Free Software\n\
Foundation.\n\
\n\
10. If you wish to incorporate parts of the Program into other free\n\
programs whose distribution conditions are different, write to the author\n\
to ask for permission.  For software which is copyrighted by the Free\n\
Software Foundation, write to the Free Software Foundation; we sometimes\n\
make exceptions for this.  Our decision will be guided by the two goals\n\
of preserving the free status of all derivatives of our free software and\n\
of promoting the sharing and reuse of software generally.\n\
\n\
NO WARRANTY\n\
\n\
11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n\
FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n\
OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n\
PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n\
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n\
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n\
TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n\
PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n\
REPAIR OR CORRECTION.\n\
\n\
12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n\
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n\
REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n\
INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n\
OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n\
TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n\
YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n\
PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n\
POSSIBILITY OF SUCH DAMAGES.\n\
\n\
END OF TERMS AND CONDITIONS";

/* GdkPixbuf RGBA C-Source image dump */

static const guint8 bl_logo_data[] = 
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (9216) */
  "\0\0$\30"
  /* pixdata_type (0x1010002) */
  "\1\1\0\2"
  /* rowstride (192) */
  "\0\0\0\300"
  /* width (48) */
  "\0\0\0""0"
  /* height (48) */
  "\0\0\0""0"
  /* pixel_data: */
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2\0\0\0\2\0\0\0\1"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0&7I\3i\227\310Fo\237\321\244Ik\2153\0\0\0\13\0\0\0\2\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1f\224\304'u\245\327\304}"
  "\255\336\377g\225\304\305\0\0\0=\0\0\0\34\0\0\0\5\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2\0\0\0\2\0\0"
  "\0\4\0\0\0\5\0\0\0\5\0\0\0\5\0\0\0\5\0\0\0\3\0\0\0\2\0\0\0\1\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\1h\230\311>~\255\334\357\205\262\340\377s"
  "\246\333\3775Mf\242\0\0\0W\0\0\0!\0\0\0\5\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2\0\0\0\3"
  "\0\0\0\4\0\0\0\4\0\0\0\3\27""6Z\10*`\241D+d\247\212+e\251\273,g\254\334"
  "-g\255\357-f\253\3540f\247\324=k\242\251Ek\227q9X|=\0\0\0\16\0\0\0\6"
  "\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0g\226\306*\201\257\335\360\216\270\342\377v\250\334"
  "\377o\242\325\373\6\11\13\223\0\0\0N\0\0\0\26\0\0\0\2\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0:Wx\10U\200\261QS\177"
  "\262\227R~\261\272Mx\251\263Ai\231\206)`\240\210.k\262\344/m\266\377"
  "/m\265\377/l\264\377.k\263\3770l\262\377K\177\273\377]\214\302\377]\215"
  "\303\377\\\213\301\377Y\207\275\377Nx\252\316*B^G\0\0\0\22\0\0\0\5\0"
  "\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0Ln\222"
  "\6x\250\330\316\224\273\343\377\202\260\337\377r\246\333\377\\\207\262"
  "\354\0\0\0\210\0\0\0;\0\0\0\13\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\1X\205\266D^\215\302\333`\220\305\377^\216\304"
  "\377[\212\300\377I}\271\3772p\267\377/o\270\377/o\270\377/o\270\377/"
  "n\267\377/m\266\3770m\265\377R\205\277\377a\221\307\377d\225\312\377"
  "d\225\312\377b\222\310\377^\215\303\377X\207\275\377P|\261\367)O}\227"
  "\4\12\22$\0\0\0\13\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\1j\233\315n\216\267\342\377\222\272\343\377v\250\334\377r\246\333"
  "\377Nq\225\336\0\0\0x\0\0\0,\0\0\0\6\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\1[\210\274cb\223\310\373f\230\315\377g\230\316"
  "\377c\224\312\377B{\275\377/o\271\377/p\272\3770p\273\3770p\273\3770"
  "p\272\377/p\272\377/o\270\377E|\274\377`\220\306\377f\227\314\377j\234"
  "\322\377j\235\322\377f\230\315\377a\222\307\377[\212\300\377U\203\271"
  "\377Gu\255\377&T\217\333\22)EL\0\0\0\24\0\0\0\5\0\0\0\1\0\0\0\0\0\0\0"
  "\0\0\0\0\0\34)6\4v\246\330\331\230\276\344\377\211\264\341\377r\246\333"
  "\377r\246\333\377Dc\202\323\0\0\0k\0\0\0$\0\0\0\5\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0V\201\263E`\220\306\374f\230\315\377l\236"
  "\323\377k\237\324\377@{\277\377/o\271\3770p\272\3770q\274\3770r\275\377"
  "0r\275\3770r\275\3770q\274\3771q\272\377X\210\300\377b\222\307\377h\231"
  "\317\377m\240\326\377n\241\326\377h\232\320\377b\223\310\377\\\214\301"
  "\377V\204\272\377P|\262\3770]\225\377)Z\232\365\32""8_z\0\0\0\34\0\0"
  "\0\10\0\0\0\1\0\0\0\0\0\0\0\0g\226\306@\206\262\340\377\227\275\344\377"
  "\200\256\336\377r\246\333\377r\246\333\377=Yu\313\0\0\0a\0\0\0\36\0\0"
  "\0\4\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0:Y|\11Y\210\275\336a\221"
  "\307\377g\230\316\377l\236\324\377E\200\302\377/n\270\3770p\272\3770"
  "q\274\3770r\276\3770s\277\3770t\300\3770s\277\3770r\276\377;w\273\377"
  "Z\211\277\377`\220\306\377e\227\314\377i\234\321\377j\234\321\377f\230"
  "\315\377a\221\307\377[\212\300\377U\203\271\377O|\261\377>i\236\377("
  "X\225\377*\\\234\375\34>j\227\0\0\0$\0\0\0\12\0\0\0\2\0\0\0\2l\235\317"
  "\235\222\272\343\377\224\274\343\377y\252\335\377r\246\333\377r\246\333"
  "\377:Up\307\0\0\0[\0\0\0\33\0\0\0\3\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\1Ny\253^Y\210\275\377^\216\304\377c\224\311\377N\205\303\377/n\267"
  "\377/o\271\3770q\273\3770r\275\3770t\300\3771u\301\3771u\302\3771u\301"
  "\3770s\277\377Cz\272\377W\206\273\377\\\214\301\377a\221\306\377c\224"
  "\311\377c\224\312\377a\221\307\377]\215\302\377X\207\274\377S\200\266"
  "\377My\257\377Hs\251\377&R\213\377+]\236\377)[\233\377\36@m\246\0\0\0"
  "(\0\0\0\13&7I\11r\245\331\347\232\277\345\377\222\272\343\377s\247\333"
  "\377r\246\333\377r\246\333\3779Sm\304\0\0\0X\0\0\0\32\0\0\0\3\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2Ny\253\254V\204\271\377Z\211\277\377"
  "W\211\301\3771n\265\377/n\267\377/o\271\3770q\274\3770s\276\3771t\301"
  "\3771v\303\3771w\304\3771v\303\3771t\300\377Fy\266\377S\201\266\377X"
  "\206\273\377[\212\277\377]\214\302\377]\214\302\377[\212\300\377X\206"
  "\274\377T\201\267\377O|\262\377Jv\254\377It\252\377*U\213\377+]\236\377"
  "*[\233\377(X\227\377\34<g\246\0\0\0)V}\245B~\256\336\377\230\276\344"
  "\377\215\267\341\377r\246\333\377r\246\333\377q\244\331\3776Oi\302\0"
  "\0\0W\0\0\0\31\0\0\0\3\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\4i\211\261"
  "\276\255\276\323\377\310\323\340\377Y\210\277\377/l\265\377/n\267\377"
  "/o\271\3770q\274\3770s\276\3771t\301\3771v\303\3771w\304\3771v\303\377"
  "1t\300\377Jz\263\377\201\241\307\377\260\305\335\377\322\336\354\377"
  "\344\353\364\377\343\353\364\377\324\340\355\377\256\304\335\377p\224"
  "\300\377Kw\255\377It\252\377It\252\3770Z\220\377)Z\232\377)Z\232\377"
  ")X\227\377(U\223\376\27""3Y\231Y\202\254\221\210\264\340\377\226\275"
  "\344\377\210\263\340\377q\245\332\377o\243\330\377n\241\326\3773Kc\300"
  "\0\0\0U\0\0\0\30\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\5\306"
  "\306\306\252\337\337\337\377\304\316\333\3770k\262\377/l\264\377/m\267"
  "\377/o\271\3770q\273\3770r\275\3770s\277\3771u\301\3771u\302\3771t\301"
  "\3770s\277\377\247\303\342\377\377\377\377\377\377\377\377\377\376\376"
  "\376\377\376\376\376\377\376\376\376\377\375\375\375\377\374\374\374"
  "\377\373\373\373\377\302\320\341\377Z\201\261\377It\252\3774^\223\377"
  "(W\225\377)Z\231\377)W\226\377(U\222\377&R\215\374T\202\264\347\220\271"
  "\342\377\223\272\342\377\202\257\335\377n\242\327\377m\240\325\377k\236"
  "\323\377-CY\273\0\0\0T\0\0\0\27\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\5\313\313\313\326\240\240\240\377Cf\217\377.j\261\377.k\264"
  "\377/m\266\377/n\270\3770p\272\3770q\274\3770r\276\3770s\277\3770s\277"
  "\3770s\277\3770r\276\377\300\322\347\377\367\367\367\377\366\366\366"
  "\377\365\365\365\377\364\364\364\377\363\363\363\377\362\362\362\377"
  "\362\362\362\377\310\310\310\377\321\321\321\377\335\342\347\377h\213"
  "\266\3778a\227\377'U\221\377)Y\230\377(W\224\377'U\221\377'S\216\377"
  ".Y\222\377\212\260\331\377\217\267\340\377~\253\332\377l\236\324\377"
  "j\235\322\377i\233\320\377$5G\263\0\0\0O\0\0\0\25\0\0\0\2\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\5\304\304\304\352\26\26\27\377*_\240\377."
  "i\260\377.k\263\377/l\265\377/n\267\377/o\271\3770p\272\3770q\274\377"
  "0r\275\3770r\275\3770r\275\3770q\274\377\277\316\340\377\354\354\354"
  "\377\353\353\353\377\352\352\352\377\351\351\351\377\350\350\350\377"
  "\347\347\347\377\224\224\224\377\5\5\5\377lll\377\267\267\267\377\326"
  "\331\335\377Rt\240\377&T\217\377(X\226\377(V\223\377'T\220\377'R\215"
  "\377&P\212\377Ck\237\377\212\262\334\377z\247\327\377i\233\320\377h\232"
  "\317\377`\220\302\372\10\14\20\230\0\0\0F\0\0\0\21\0\0\0\2\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\6\274\274\274\350*7F\377-g\255\377.i\257"
  "\377.j\261\377.k\263\377/m\265\377/n\267\377/o\270\377/o\272\3770p\272"
  "\3770p\273\3770p\272\377/o\271\377\254\277\325\377\340\340\340\377\337"
  "\337\337\377\336\336\336\377\335\335\335\377\334\334\334\377\333\333"
  "\333\377SSS\377\2\2\2\377EEE\377xxx\377\326\326\326\377\212\235\265\377"
  "$P\211\377(W\225\377(U\222\377'S\216\377&Q\213\377&O\210\377%M\205\377"
  "^\206\266\377x\244\324\377g\230\315\377e\226\314\377\77_\201\336\0\0"
  "\0\201\0\0\0""5\0\0\0\12\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\5\261\261\261\317y\221\260\377-f\254\377-h\256\377.i\260\377.j\262"
  "\377.k\264\377/l\265\377/m\266\377/n\267\377/n\270\377/o\270\377/n\270"
  "\377/n\267\377\212\246\307\377\325\325\325\377\324\324\324\377\323\323"
  "\323\377\322\322\322\377\321\321\321\377\320\320\320\377\243\243\243"
  "\377\25\25\25\377!!!\377\265\265\265\377\313\313\313\377\225\252\304"
  "\377(O\202\377'U\221\377(T\220\377&R\215\377&P\212\377%N\207\377%L\204"
  "\377*P\206\377n\231\312\377d\225\312\377c\223\311\377\27#/\264\0\0\0"
  "f\0\0\0\"\0\0\0\5\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\5\234"
  "\234\234\234_\202\255\377-e\253\377-g\255\377.h\256\377.i\260\377.j\262"
  "\377.k\263\377/l\264\377/l\265\377/m\266\377/m\266\377/m\266\377/l\265"
  "\377a\213\274\377\311\311\311\377\310\310\310\377\307\307\307\377\306"
  "\306\306\377\305\305\305\377\304\304\304\377\303\303\303\377\302\302"
  "\302\377\301\301\301\377\300\300\300\377\276\277\300\377~\234\301\377"
  "4\\\217\377$M\205\377'S\216\377&Q\213\377&O\210\377%M\205\377%K\202\377"
  "$I\200\377Bk\237\377a\222\307\377T~\256\364\0\0\0\223\0\0\0H\0\0\0\22"
  "\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\3mmmFJt\247\376"
  "-d\251\377-f\253\377-g\255\377.h\256\377.i\260\377.j\261\377.j\262\377"
  ".k\263\377.k\263\377.k\263\377.k\263\377.k\263\377Fx\265\377\267\273"
  "\277\377\275\275\275\377\274\274\274\377\273\273\273\377\272\272\272"
  "\377\271\271\271\377\270\270\270\377\267\267\267\377\266\266\266\377"
  "\265\265\265\377\243\255\272\377f\212\267\3779b\225\377\"H|\377&R\215"
  "\377&P\212\377&N\207\377$L\204\377$J\201\377#H~\377%I~\377Z\211\277\377"
  "@`\205\340\0\0\0~\0\0\0""2\0\0\0\10\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\2\0\0\0\21,_\235\342,c\250\377-e\251\377-f\253\377"
  "-g\254\377-h\256\377.h\257\377.i\260\377.i\260\377.j\261\377.j\261\377"
  ".j\261\377.i\260\377:o\256\377\202\233\271\377\261\261\261\377\260\260"
  "\260\377\257\257\257\377\256\256\256\377\255\255\255\377\254\254\254"
  "\377\253\253\253\377\252\252\252\377\247\250\252\377z\227\272\377Oy\255"
  "\3777_\222\377!Fx\377&Q\213\377%O\210\377$M\205\377$K\202\377$I\177\377"
  "#G|\377#Fy\377Eo\244\377/Hd\315\0\0\0k\0\0\0%\0\0\0\5\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\11*^\236\343,b\246\377"
  ",c\247\377-d\251\377-e\252\377-f\254\377-g\255\377-g\255\377-h\256\377"
  ".h\256\377.h\257\377.h\256\377-h\256\3773j\255\377Pz\256\377\214\233"
  "\257\377\244\244\244\377\243\243\243\377\242\242\242\377\241\241\241"
  "\377\240\240\240\377\240\240\240\377\236\236\237\377}\223\260\377Y\200"
  "\261\377It\252\377.T\206\377!Fx\377%O\210\377%N\206\377$L\203\377$J\200"
  "\377$H}\377#F{\377\"Dx\3777^\221\377$7M\276\0\0\0\\\0\0\0\34\0\0\0\3"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\6*\\\235"
  "\335,a\244\377,b\246\377,c\247\377,d\250\377-e\251\377-e\252\377-f\253"
  "\377-f\254\377-f\254\377-f\254\377-f\254\377-f\254\377-f\253\377Cq\252"
  "\377Qz\256\377}\221\253\377\226\227\232\377\227\227\227\377\226\226\226"
  "\377\225\225\225\377\217\223\231\377v\217\257\377Z\201\262\377It\252"
  "\377Eo\245\377!Ds\377!H|\377%N\206\377$L\204\377$K\201\377$I~\377#G{"
  "\377\"Ey\377\"Dv\3778^\222\377&;T\276\0\0\0T\0\0\0\30\0\0\0\2\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\5(Z\231\321+`"
  "\242\377,a\244\377,b\245\377,b\246\377,c\247\377,d\250\377-d\251\377"
  "-e\251\377-e\252\377-e\252\377-e\252\377-e\251\377,d\250\3771f\247\377"
  "Hs\251\377Lv\253\377[\200\257\377n\210\251\377u\213\247\377o\211\254"
  "\377^\203\263\377Py\255\377It\252\377It\252\377,Q\202\377\35Ao\377$M"
  "\204\377%M\204\377$K\202\377$I\177\377#H|\377\"Fy\377\"Dw\377!Bt\377"
  "Aj\236\377-Fd\307\0\0\0T\0\0\0\30\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\5'V\222\266+_\240\377+_\242\377,`"
  "\243\377,a\244\377,b\245\377,b\246\377,c\246\377,c\247\377,c\247\377"
  ",c\247\377,c\247\377+b\246\377+a\244\377+`\243\377.[\223\377Fq\246\377"
  "It\252\377It\252\377It\252\377It\252\377It\252\377It\252\377Gr\250\377"
  ".T\206\377\35An\377!Gz\377%M\205\377$L\202\377$J\200\377#H}\377\"Gz\377"
  "\"Ex\377\"Cu\377\"Bt\377My\256\3777Ty\327\0\0\0\\\0\0\0\34\0\0\0\3\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\4%P\210"
  "\222+]\236\377+^\240\377+_\241\377+`\242\377,`\243\377,a\243\377,a\244"
  "\377,a\245\377,b\245\377,a\245\377+a\244\377+`\242\377+_\241\377*^\237"
  "\377'X\225\377$M\201\3775_\223\377Bm\242\377Hs\251\377Hs\251\377Cm\242"
  "\3777_\222\377$Gx\377\36Ao\377!G{\377%M\205\377$L\202\377#J\200\377#"
  "H~\377\"G{\377\"Ex\377\"Cv\377!Bs\3770T\207\377O|\262\377Bg\225\354\0"
  "\0\0i\0\0\0#\0\0\0\5\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\3\40EwZ*\\\234\377+]\236\377+]\237\377+^\240\377+_\240"
  "\377+_\241\377+_\242\377+`\242\377+`\242\377+_\241\377*^\240\377*^\237"
  "\377*]\236\377)\\\234\377)[\232\377'V\223\377#M\204\377!Gz\377\40Fw\377"
  "\40Ev\377\37Cr\377\40Du\377!H{\377$M\204\377%N\205\377%L\203\377#J\200"
  "\377#I~\377#G{\377\"Ey\377\"Dv\377!Bt\377\40Ar\377Dn\243\377My\257\377"
  "Jv\254\376\20\32'\214\0\0\0""0\0\0\0\10\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2\16\37""5\35)Y\227\367*[\233\377*\\"
  "\234\377+]\235\377+]\236\377+^\237\377+^\237\377+^\237\377*]\236\377"
  "*]\236\377*\\\234\377)[\234\377)[\232\377)Z\231\377)Y\227\377(X\226\377"
  "'W\224\377'U\222\377'T\216\377&Q\213\377&Q\212\377%P\212\377%O\207\377"
  "%M\205\377$L\203\377#J\200\377#I~\377#G|\377\"Ey\377\"Dw\377!Ct\377\40"
  "Ar\377/S\205\377Lw\255\377Ju\253\377It\252\3774Ry\321\0\0\0G\0\0\0\22"
  "\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2h\227"
  "\310tO\201\273\373+Z\232\377*Z\232\377*[\233\377*\\\234\377*\\\235\377"
  "*\\\234\377*[\234\377*[\233\377*[\232\377)Z\231\377)Y\230\377)X\227\377"
  "(X\225\377(W\224\377'V\223\377'U\221\3778g\241\377W\210\277\3775b\233"
  "\377%P\211\377%N\207\377%M\205\377$L\202\377#J\200\377#I~\377#G|\377"
  "\"Fy\377\"Dw\377!Ct\377\40Ar\377\36=k\373/Ms\346It\252\377It\252\377"
  "It\252\377Hs\250\376!4M\221\0\0\0)\0\0\0\10\0\0\0\1\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0V~\250\22o\243\330\370p\244\331\377_\222\311\377"
  ";k\250\377*Z\231\377*Z\232\377)Z\231\377)Y\231\377)Y\230\377)Y\227\377"
  ")X\227\377(X\226\377(W\225\377(W\223\377(V\223\3773a\234\377Iy\261\377"
  "b\224\312\377n\241\326\377m\241\326\377)T\215\377%N\206\377%M\204\377"
  "$K\202\377#J\200\377#H}\377#G{\377\"Fy\377\"Dw\377!Bu\377\40Ar\377\40"
  "@p\377\16\34""2\317\0\0\0\230%;V\262Fo\243\371It\252\377It\252\377Fo"
  "\243\367\35.Cq\0\0\0\36\0\0\0\6\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0Hl\221\30d\225\312\375e\226\313\377d\225\313\377d\225\313\377[\214"
  "\302\377Jz\263\377=m\250\3777e\242\3773b\236\3772`\235\3774b\236\377"
  "9h\243\377Bp\251\377L{\263\377Z\212\300\377a\222\307\377a\222\307\377"
  "b\222\310\377j\234\321\377Jy\257\377%M\205\377%L\203\377$K\201\377#J"
  "\177\377#H}\377#G{\377\"Fy\377\"Dw\377!Bu\377\40Ar\377\40@p\377\26,M"
  "\344\0\0\0\226\0\0\0a\0\0\0D\23\37-l9Z\205\324Hr\247\376It\252\377Fo"
  "\243\365$9Tm\0\0\0\31\0\0\0\5\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\10"
  "Ox\247\275W\205\273\377W\205\273\377W\205\273\377W\205\272\377V\204\272"
  "\377V\204\272\377V\204\271\377V\204\271\377U\203\271\377U\203\271\377"
  "U\203\270\377U\202\270\377U\202\270\377T\202\270\377U\202\270\377[\211"
  "\277\377d\225\312\377Y\210\276\377'O\206\377$L\202\377#J\200\377#I~\377"
  "#H}\377#F{\377\"Ex\377!Dv\377!Bt\377\40Ar\377\40@p\377\32""3[\360\1\3"
  "\5\242\0\0\0h\0\0\0.\0\0\0\24\0\0\0\36\0\0\0@\17\30$v$9T\255*Cc\266#"
  "9S\212\0\0\0,\0\0\0\13\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\5Ls\235"
  "\206R}\255\377Jt\246\377Kv\253\377Kv\254\377Kv\254\377Kv\254\377Jv\254"
  "\377Jv\254\377Jv\254\377Ju\253\377Ju\253\377Jt\251\377Lv\251\377R~\262"
  "\377Z\211\275\377]\215\303\377W\206\273\377+S\212\377$K\201\377#I\177"
  "\377#I~\377#G|\377\"Fz\377\"Ex\377!Cv\377!Bt\377\40Ar\377\40@p\377\34"
  "6`\364\4\7\15\255\0\0\0t\0\0\0""7\0\0\0\17\0\0\0\3\0\0\0\5\0\0\0\21\0"
  "\0\0%\0\0\0;\0\0\0I\0\0\0A\0\0\0%\0\0\0\13\0\0\0\1\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\2';R*V\203\267\362T\201\264\377Lu\245\377Hp\237\377Fm\234"
  "\377Dj\231\377Dk\231\377El\234\377Go\237\377Js\243\377Nx\251\377S\177"
  "\262\377V\203\271\377V\203\271\377U\203\271\377N{\261\377+S\212\377$"
  "J\200\377#I~\377#H}\377\"G{\377\"Ey\377\"Ew\377!Cu\377!Bs\377\40Aq\377"
  "\40\77o\377\34""6a\365\5\12\21\261\0\0\0y\0\0\0>\0\0\0\24\0\0\0\3\0\0"
  "\0\0\0\0\0\0\0\0\0\2\0\0\0\5\0\0\0\14\0\0\0\23\0\0\0\25\0\0\0\15\0\0"
  "\0\4\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\14""3OqnNz\257\373"
  "O|\262\377O|\262\377O|\261\377O{\261\377N{\261\377N{\261\377Nz\260\377"
  "Nz\260\377Mz\260\377Mz\257\377My\257\377My\257\377\77i\237\377'M\203"
  "\377$I~\377#H}\377\"G{\377\"Fz\377\"Ex\377\"Dv\377!Cu\377!As\377\40A"
  "q\377\40\77o\377\33""4\\\362\4\10\17\256\0\0\0z\0\0\0A\0\0\0\27\0\0\0"
  "\5\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\2\0"
  "\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\4\0\0\0"
  "\31&<XxCk\235\357It\252\377It\252\377It\252\377It\252\377It\252\377I"
  "t\252\377It\252\377It\252\377Hs\251\377>g\235\377,S\211\377$I~\377#H"
  "}\377\"G{\377\"Fz\377\"Ey\377\"Dw\377\"Cu\377!Bt\377\40Ar\377\40@p\377"
  "\40>n\377\27-P\350\2\4\7\245\0\0\0u\0\0\0\77\0\0\0\27\0\0\0\5\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\7"
  "\0\0\0\36\5\11\15M$:U\2434Sz\325;^\212\347@g\232\367Ak\241\377\77h\236"
  "\377:b\230\3771Y\217\377'M\202\377#H}\377#G|\377#G{\377\"Fz\377\"Ex\377"
  "\"Dw\377\"Cv\377!Bt\377\40Ar\377\40@q\377\40\77o\377\37<i\375\20\37""7"
  "\323\0\0\0\227\0\0\0k\0\0\0""9\0\0\0\25\0\0\0\5\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0"
  "\7\0\0\0\27\0\0\0""1\0\0\0N\0\0\0e\4\10\17\177\32""5]\321#G|\376#G}\377"
  "#G|\377#F{\377#Fz\377#Fy\377\"Ex\377\"Dw\377\"Cu\377!Bt\377!Bs\377\40"
  "Aq\377\40\77p\377\40\77n\377\30/S\351\6\14\26\264\0\0\0\207\0\0\0[\0"
  "\0\0.\0\0\0\20\0\0\0\3\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\3\0\0\0"
  "\12\0\0\0\25\0\0\0!\0\0\0,\0\0\0A\15\33/\206\34""8a\334\"Dx\376#Ex\377"
  "\"Dw\377!Cv\377!Cu\377!Bt\377!Ar\377!Aq\377\40@p\377\37>m\376\31""0U"
  "\353\12\24$\277\0\0\0\221\0\0\0n\0\0\0D\0\0\0!\0\0\0\13\0\0\0\2\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2\0\0\0\4\0"
  "\0\0\7\0\0\0\20\0\0\0'\0\0\0H\10\20\35\177\23&D\274\31""3Z\337\35:f\361"
  "\37=l\371\37<k\371\35""8b\364\30/T\347\20\40:\316\6\13\24\251\0\0\0\213"
  "\0\0\0p\0\0\0M\0\0\0+\0\0\0\23\0\0\0\6\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\2\0\0\0\7\0\0\0\24\0\0\0(\0\0\0\77\0\0\0X\0\0\0m\0\0\0{\0\0\0\202"
  "\0\0\0\203\0\0\0{\0\0\0n\0\0\0Y\0\0\0A\0\0\0*\0\0\0\25\0\0\0\10\0\0\0"
  "\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2\0\0\0"
  "\6\0\0\0\16\0\0\0\32\0\0\0%\0\0\0,\0\0\0""0\0\0\0""1\0\0\0-\0\0\0%\0"
  "\0\0\32\0\0\0\16\0\0\0\7\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0"
  "\0\4\0\0\0\5\0\0\0\6\0\0\0\6\0\0\0\5\0\0\0\4\0\0\0\2\0\0\0\1\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0"};

static GtkWidget *info;

static GtkWidget *
create_header (GdkPixbuf * icon, const char *text)
{
    GtkWidget *eventbox, *label, *hbox, *image;
    GtkStyle *style;
    char *markup;
    GdkColormap *cmap;
  	GdkColor color;

    eventbox = gtk_event_box_new ();

    hbox = gtk_hbox_new (FALSE, 12);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
    gtk_container_add (GTK_CONTAINER (eventbox), hbox);

    if (icon) {
		image = gtk_image_new_from_pixbuf (icon);
		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
    }

    style = gtk_widget_get_style (eventbox);
    cmap = gdk_colormap_get_system ();
 	color.red = 0xaaaa;
  	color.green = 0x1112;
  	color.blue = 0x3fea;
  	if (!gdk_color_alloc (cmap, &color)) {
    	g_print ("couldn't allocate color");
  	}
    gtk_widget_modify_bg (eventbox, GTK_STATE_NORMAL, &color);

    markup = g_strconcat ("<span size=\"larger\"  weight=\"bold\">", text, "</span>", NULL);
    label = gtk_label_new (markup);
    g_free (markup);
    gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
 
    style = gtk_widget_get_style (label);
    gtk_widget_modify_fg (label, GTK_STATE_NORMAL,
			  &style->fg[GTK_STATE_SELECTED]);

	gtk_widget_show_all (eventbox);
	return eventbox;
}

static GdkPixbuf *
inline_icon_at_size (const guint8 * data, int width, int height)
{
    GdkPixbuf *base;

    base = gdk_pixbuf_new_from_inline (-1, data, FALSE, NULL);

    g_assert (base);

    if ((width < 0 && height < 0) || (gdk_pixbuf_get_width (base) == width
	    && gdk_pixbuf_get_height (base) == height)) {
		return base;
    } else {
		GdkPixbuf *scaled;

		scaled = gdk_pixbuf_scale_simple (base,
					     				  width >
					     				  0 ? width : gdk_pixbuf_get_width (base),
					     				  height >
				    	 				  0 ? height :
				     					  gdk_pixbuf_get_height (base),
				     					  GDK_INTERP_BILINEAR);

		g_object_unref (G_OBJECT (base));

		return scaled;
    }
}

static void
add_page (GtkNotebook * notebook, const gchar * name, const gchar * buf, gboolean hscrolling)
{
    GtkTextBuffer *textbuffer;
    GtkWidget *textview;
    GtkWidget *label;
    GtkWidget *view;
    GtkWidget *sw;
 
    label = gtk_label_new (name);

	view = gtk_frame_new (NULL);
	gtk_container_set_border_width (GTK_CONTAINER (view), BORDER);
	gtk_frame_set_shadow_type (GTK_FRAME (view), GTK_SHADOW_IN);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
									hscrolling ? GTK_POLICY_AUTOMATIC :
									GTK_POLICY_NEVER,
									GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (view), sw);

	textbuffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_set_text (textbuffer, buf, strlen(buf));

	textview = gtk_text_view_new_with_buffer (textbuffer);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), FALSE);
	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (textview), BORDER);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (textview), BORDER);
	
	gtk_container_add (GTK_CONTAINER (sw), textview);

	gtk_notebook_append_page (notebook, view, label);
	
	gtk_widget_show_all (view);
}

void
about_dialog_create (gpointer *data, guint *callback_action, GtkWidget *widget)
{
    GtkWidget *header;
    GtkWidget *vbox, *vbox2;
    GtkWidget *notebook;
    GtkWidget *info_ok_button;
    GdkPixbuf *logo_pb;
    char *text;
  
    info = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (info), "About Bluefish");
	gtk_window_set_resizable (GTK_WINDOW (info), FALSE);
	gtk_window_set_modal (GTK_WINDOW (info), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (info), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_position (GTK_WINDOW (info), GTK_WIN_POS_CENTER);

    logo_pb = inline_icon_at_size (bl_logo_data, 58, 58);
    gtk_window_set_icon (GTK_WINDOW (info), logo_pb);

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (info), vbox2);
	gtk_widget_show (vbox2);

    /* header with logo */
    text = g_strdup_printf
		("%s\n<span size=\"smaller\" style=\"italic\" color=\"#6872fe\">%s %s\n%s</span>",
	 	"Bluefish Web Development Studio",
		"Version", VERSION,
	 	"Copyright (c) 1998-2004 The Bluefish Team");
    header = create_header (logo_pb, text);
    gtk_box_pack_start (GTK_BOX (vbox2), header, FALSE, FALSE, 0);
	gtk_widget_show (header);
    g_free (text);
    g_object_unref (logo_pb);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
    gtk_box_pack_start (GTK_BOX (vbox2), vbox, TRUE, TRUE, 0);
    gtk_widget_show (vbox);
	
    /* the notebook */
    notebook = gtk_notebook_new ();

    /* add pages */
    add_page (GTK_NOTEBOOK (notebook), "Info", INFO, FALSE);
    add_page (GTK_NOTEBOOK (notebook), "Authors", AUTHORS, FALSE);
   	add_page (GTK_NOTEBOOK (notebook), "Translators", TRANSLATORS, TRUE);
    add_page (GTK_NOTEBOOK (notebook), "License", LICENSE, TRUE);
	
    gtk_widget_set_size_request (notebook, -1, 300);
	gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
	gtk_widget_show (notebook);	
	
    /* button */
    info_ok_button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	GTK_WIDGET_SET_FLAGS (info_ok_button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (vbox), info_ok_button, FALSE, FALSE, 0);
	gtk_widget_show (info_ok_button);
	gtk_widget_grab_default (info_ok_button);

    g_signal_connect (info, "destroy-event", G_CALLBACK (close_modal_window_lcb), info);
    g_signal_connect (info_ok_button, "clicked", G_CALLBACK (close_modal_window_lcb), info);
 
    gtk_widget_show (info);
	
	gtk_main ();
}
