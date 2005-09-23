/* Bluefish HTML Editor
 * fref.h - function reference file
 *
 * Copyright (C) 2003 Oliver Sessink and Oskar Swida
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* ---------------------------------------------------------- */
/* -------------- XML FILE FORMAT --------------------------- */
/* ---------------------------------------------------------- */

/* 
<ref name="MySQL Functions" description="MySQL functions for PHP " case="1">

<group name="some_function_group">
  <description>
     Group description
  </description>
      
        <group name="Groups can be nested">
           <function ...>
           </function>
        </group>

<function name="mysql_db_query">

   <description>
     Function description 
   </description>

   <tip>Text shown in a tooltip or hints</tip>
      
   <param name="database" title="Database name" required="1" vallist="0" default="" type="string" >
     <vallist>Values if vallist==1</vallist>
     Parameter description
   </param>       
    
   <return type="resource">
     Return value description
   </return>
  
   <dialog title="Dialog title">
      Text inserted after executing dialog, use params as %0,%1,%2 etc.
      %_ means "insert only these attributes which are not empty and not 
                default values" 
   </dialog>
   
   <insert>
      Text inserted after activating this action
   </insert>
   
   <info title="Title of the info window">    
     Text shown in the info
   </info>    
   
 </function>
 
 <tag name="Table Element">
 
   <description>
       The TABLE element contains all other elements that specify caption, rows, content, and formatting.
   </description>

   <tip>Tag tooltip</tip>
     
   <attribute name="Border" title="Table border" required="0" vallist="0" default="0">
      <vallist></vallist>
          This attribute specifies the width (in pixels only) of the frame around a table (see the Note below for more information about this attribute).
   </attribute>

   <dialog title="Dialog title">
      Text inserted after executing dialog, use params as %0,%1,%2 etc.
   </dialog>
   
   <insert>
      Text inserted after activating this action
   </insert>
   
   <info title="Title of the info window">    
     Text shown in the info
   </info>    
  
 </tag>
   
</group>

</ref>
*/

#ifndef __FREF_H__
#define __FREF_H__

GtkWidget *fref_gui(Tbfwin *bfwin); /* used in gui.c */

void fref_init(void); /* only used once */
void fref_cleanup(Tbfwin *bfwin);
void fref_rescan_dir(const gchar *dir); /* used by rcfile.c */

#endif /* __FREF_H__ */


