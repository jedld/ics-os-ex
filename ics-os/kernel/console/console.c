/* 
   ==========================================================================
   Console.c
   Author: Joseph Emmanuel Dayo
   Date updated:December 6, 2002
   Description: A kernel mode console that is used for debugging the kernel
   and testing new kernel features.
                
    DEX educational extensible operating system 1.0 Beta
    Copyright (C) 2004  Joseph Emmanuel DL Dayo

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
   ==========================================================================
*/

#include "console.h"


void runner(){
   int i=0;
   while(1){
      i++;
      i--;
   }
   //printf("Hello user thread!\n");
}

  
/*A console mode get string function terminates
upon receving \r */
void getstring(char *buf, DEX32_DDL_INFO *dev){
   unsigned int i=0;
   char c;
   if (current_process && current_process->ctty) {
      int n = tty_read(current_process->ctty, buf, 255);
      if (n < 0) {
         buf[0] = 0;
         return;
      }
      if (n > 0 && buf[n-1] == '\n')
         n--;
      buf[n] = 0;
      return;
   }
   do{
      c=getch();
      if (c=='\r' || c=='\n' || c==0xa) 
         break;

      if (c=='\b' || (unsigned char)c == 145){
         if(i>0){
            i--;
            if (Dex32GetX(dev)==0){
               Dex32SetX(dev,79);
               if (Dex32GetY(dev)>0) 
                  Dex32SetY(dev,Dex32GetY(dev)-1);
            }else{
               Dex32SetX(dev,Dex32GetX(dev)-1);
            }     
            Dex32PutChar(dev,Dex32GetX(dev),Dex32GetY(dev),' ',Dex32GetAttb(dev));
         };
      }else{
         if (i<256){  //maximum command line is only 255 characters
            Dex32PutChar(dev,Dex32GetX(dev),Dex32GetY(dev),buf[i]=c,Dex32GetAttb(dev));
            i++;
            Dex32SetX(dev,Dex32GetX(dev)+1);     
            if (Dex32GetX(dev)>79){
               Dex32SetX(dev,0);
               Dex32NextLn(dev);
            };
         };
      };

      Dex32PutChar(dev,Dex32GetX(dev),Dex32GetY(dev),' ',Dex32GetAttb(dev));
      update_cursor(Dex32GetY(dev),Dex32GetX(dev));
   }while (c!='\r');
    
   Dex32SetX(dev,0);
   Dex32NextLn(dev);
   buf[i]=0;
};

/*Show information about memory usage. This function is also useful
  for detecting memory leaks*/
void meminfo(){
   DWORD totalbytes = totalpages * 0x1000;
   DWORD freebytes = totalbytes - (totalpages - stackbase[0])* 0x1000;
   printf("=================Memory Information===============\n");
   printf("Pages available     : %10u pages\n",stackbase[0]);
   printf("Total Pages         : %10d pages\n",totalpages);
   printf("Total Memory        : %10u bytes (%10d KB)\n",totalbytes, totalbytes / 1024);
   printf("Free Memory         : %10u bytes (%10d KB)\n",freebytes, freebytes / 1024);
   printf("Used pages          : %10d pages (%10d KB)\n",totalpages-stackbase[0],
   (totalpages-stackbase[0])*0x1000);
};


int delfile(char *fname){
   int sectors;
   file_PCB *f=openfilex(fname,FILE_WRITE);
   return fdelete(f);
};


/*
 * Forks a new process
 */
int user_fork(){
   int curval = current_process->processid;

   int childready = 0, retval = 0;
   int hdl;
   int id;
   DWORD flags;

   #ifdef DEBUG_FORK
   printf("user_fork called\n");
   #endif
    
   //enable interrupts since we want the process dispatcher to take control
   storeflags(&flags);
   startints();
   
   //Calls pd_forkmodule() from kernel/process/pdispatch.c 
   hdl = pd_forkmodule(current_process->processid);

   //inform the CPU scheduler, hopefully to schedule process_dispatcher() 
   taskswitch();  

   //id = pd_ok(hdl); 
   id = pd_dispatched(hdl);
   while (!(id = pd_dispatched(hdl))){ //wait for the process to be dispatched before returning
      taskswitch(); //try to wakeup process_dispatcher() 
      ; 
   }

   if (curval != current_process->processid){ //this is the child
      //If this is the child process, the processid when this function
      //was called is not equal to the current processid.
      //pd_ok(hdl);
      retval = 0;
   };
      
   if (curval == current_process->processid){ // this is the parent
      pd_ok(hdl);          //free the createp_queue node used by the child
      retval = id;
   };
      
   restoreflags(flags);
   return retval;
};


#include "selfhost.c"  /* tccboot / kbuild / fullhost self-host drivers */

/**
 * Function that reads an executable and creates a new process for it.
 */
int user_execp(char *fname, DWORD mode, char *params){
   static char cached_name[256];
   static char *cached_buf;
   static DWORD cached_size;
   DWORD id,size;
   char *buf;
   int from_cache = 0;

   if (cached_buf && strcmp(cached_name, fname) == 0){
      buf = cached_buf;
      size = cached_size;
      from_cache = 1;
   } else {
      /* File-backed eager mmap: one contiguous fill via coalesced FAT I/O. */
      buf = (char*)vfs_mapfile(fname, &size);
      if (!buf)
         return 0;
   }

   printf("execp: loading %s (%u bytes)%s\n", fname, (unsigned)size,
          from_cache ? " [cached]" : " [mmap]");
   {
         char temp[255];

         /* Load synchronously from the caller. The historic process_dispatcher
            kthread path can starve under software scheduling while the console
            spins on pd_ok(). */
         id = dex32_loader(fname, buf, userspace, mode, params,
                          showpath(temp), current_process);

         if (!id || (int)id == -1){
            printf("execp: failed to start %s\n", fname);
            if (!from_cache){
               free(buf);
               cached_buf = 0;
               cached_name[0] = 0;
            }
            return 0;
         }
         printf("execp: started pid=%d, waiting\n", (int)id);
         fg_setmykeyboard(id);
         dex32_child_faulted = 0;
         //wait for the child process to finish
         dex32_waitpid(id,0);

         fg_setmykeyboard(getprocessid());
         if (!from_cache){
            /* Do not cache ELF images across runs: user processes share
               pagedir1 and a stale buffer can be left inconsistent after exit. */
            free(buf);
         }
         if (dex32_child_faulted) {
            printf("execp: child faulted\n");
            return 0;
         }
         return id;
   };
};

int exec(char *fname, DWORD mode, char *params){
   DWORD id;
   char *buf;
   file_PCB *f=openfilex(fname,0);

   if (f!=0){
      DWORD size;
      char temp[255];
      vfs_stat fileinfo;
      fstat(f,&fileinfo);
      size = fileinfo.st_size;
     
      buf=(char*)malloc(size+511);
     
      if (fread(buf,size,1,f) == size)
         id=dex32_loader(fname, buf, userspace, mode, params, showpath(temp), getprocessid());
      
      free(buf);
      fclose(f);
      return id;
   };
   return 0;
;};

int user_exec(char *fname,DWORD mode,char *params){
   DWORD id;
   char *buf;
   file_PCB *f = openfilex(fname, FILE_READ);

   if (f != 0){
      int hdl,size;
      char temp[255];
      vfs_stat fileinfo;
      fstat(f, &fileinfo);
      size = fileinfo.st_size;
      buf=(char*)malloc(size+511);
      fread(buf, size, 1, f);
      hdl = addmodule(fname, buf, userspace, mode, params, showpath(temp), getprocessid());
      while (!pd_ok(hdl)) 
         ;
     
      free(buf);
      fclose(f);
      return id;
  };
  return 0;
};


int loadDLL(char *name, char *p){
   file_PCB *handle;
   int fsize; vfs_stat filestat;
   int hdl,libid;
   char *buf;

   handle=openfilex(name,FILE_READ);
   if (!handle) 
      return -1;
   fstat(handle,&filestat);
   vfs_setbuffer(handle,0,filestat.st_size,FILE_IOFBF);
   //get filesize and allocate memory
   fsize= filestat.st_size;
   buf=(char*)malloc(fsize);
 
   //load the file from the disk 
   fread(buf, fsize, 1, handle);
 
   /*Tell the process dispatcher to map the file into memory and
      create data structures necessary for managing dynamic libraries*/
   hdl = addmodule(name, buf, lmodeproc, PE_USERDLL, p, 0, getprocessid());
 
   //wait until the library has been loaded before we continue since addmodule returns immediately
   while (!(libid = pd_ok(hdl)))
      ;
 
   //done!
   free(buf);

   //close the file
   fclose(handle);
   return libid;
};

void loadfile(char *s,int,int);

void loadlib(char *s){
   char *buf;
   DWORD size;
   loadDLL(s,0);
};


int console_showfile(char *s, int wait){
   char *buf;
   DWORD size;
   file_PCB *handle;
   vfs_stat fileinfo;
   int i;
   DEX32_DDL_INFO *myddl;
   handle=openfilex(s, FILE_READ);

   if (!handle) 
      return -1;
   fstat(handle,&fileinfo);
   size = fileinfo.st_size;
   buf=(char*)malloc(size);
   textbackground(BLUE);
   myddl = Dex32GetProcessDevice();
   printf("Name: %s  Size: %d bytes \n", s, size);
   textbackground(BLACK);
   fread(buf, size, 1, handle);
   for (i=0; i<size; i++){
      if (buf[i]!='\r') 
         printf("%c",buf[i]);
      if (myddl->lines%25==0){ 
         char c;
         printf("\nPress any key to continue, 'q' to quit\n");
         c=getch();
         if (c=='q') 
            break;
      };
   };
   fclose(handle);
   free(buf);
   return 1;
};


//creates a virtual console for a process
DWORD alloc_console(){
   dex32_commit(0xB8000, 1, current_process->pagedirloc, PG_WR);
};

void console(){
   console_main();
};


void prompt_parser(const char *promptstr, char *prompt){
   int i, i2 = 0, i3 = 0;
   char command[10], temp[255];
   strcpy(prompt,"");
   for (i=0; promptstr[i] && i<255; i++){
      if (promptstr[i] != '%'){ //add to the prompt
         prompt[i2]=promptstr[i];
         i2++;
         prompt[i2]=0;
      }else{
         if (promptstr[i+1] != 0){
            if (promptstr[i+1] == '%'){
               prompt[i2] = '%';
               i2++;
               prompt[i2] = 0;
               i+=2;
               continue;
            };
         };
         i3=0;
         for (i2=i+1; promptstr[i2]&&i2 < 255; i2++){
            if (promptstr[i2] == '%' || i3 >= 10) 
               break;
            command[i3]=promptstr[i2];
            i3++;
         };
         i=i2;
         command[i3]=0;
         if (strcmp(command,"cdir")==0){
            strcat(prompt, showpath(temp));
            i2=strlen(prompt);
         };
      };
   };
};
  

void kernel_file_io_demo(){
   char msg[10]="Hello!";  
   char buf[10];  
   file_PCB *fp;

   fp=openfilex("sample.txt",FILE_WRITE);
   fwrite(msg,sizeof(msg),1,fp);
   fclose(fp);
   
   fp=openfilex("sample.txt",FILE_READ);
   fread(buf,sizeof(msg),1,fp);
   printf("%s\n",buf);
   fclose(fp);

}



/*An auxillary function for qsort for comparing two elements, based on size*/
int console_ls_sortsize(vfs_node *n1, vfs_node *n2){
   if (n1->size > n2->size) 
      return -1;
   if (n2->size > n1->size) 
      return 1;
   return 0;
};

/*An auxillary function for qsort for comparing two elements, based on name*/
int console_ls_sortname(vfs_node *n1, vfs_node *n2){
   if ( (n1->attb & FILE_DIRECTORY) && !(n2->attb & FILE_DIRECTORY))
      return -1;

   if ( !(n1->attb & FILE_DIRECTORY) && (n2->attb & FILE_DIRECTORY))
      return 1;
        
   return strsort(n1->name,n2->name);
};

/* ==================================================================
   console_ls(int style):
   
   *list the contents of the current directory to the screen
    style = 1      : list format 
    style = others : wide format  
*/
/*lists the files in the current directory to the console screen*/
void console_ls(int style, int sortmethod){
   vfs_node *dptr=current_process->workdir;
   vfs_node *buffer;
   int totalbytes=0, freebytes=0;
   int totalfiles=0, i;
   char cdatestr[20], mdatestr[20], temp[20];
    
   //obtain total number of files
   totalfiles = vfs_listdir(dptr, 0, 0);
    
   buffer = (vfs_node*) malloc( totalfiles * sizeof(vfs_node));

   //Place the list of files obtained from the VFS into a buffer
   totalfiles = vfs_listdir(dptr, buffer, totalfiles * sizeof(vfs_node));     
    
   //Sort the list
   if (sortmethod == SORT_NAME)
      qsort(buffer, totalfiles, sizeof(vfs_node), console_ls_sortname);
   else if (sortmethod == SORT_SIZE)
      qsort(buffer, totalfiles, sizeof(vfs_node), console_ls_sortsize);
        
   textbackground(BLUE);
   textcolor(WHITE);
    
   if (style==1)
      printf("%-25s %10s %14s %14s\n","Filename", "Size(bytes)", "Attribute", "Date Modified");
        
   textbackground(BLACK);

   for (i=0; i < totalfiles; i++){
      char fname[255];   
      if (style == 0){ //wide view style
         if (buffer[i].attb&FILE_MOUNT)
            textcolor(LIGHTBLUE);
         else if (buffer[i].attb&FILE_DIRECTORY)
            textcolor(GREEN);
         else if (buffer[i].attb&FILE_OEXE)
            textcolor(YELLOW);
         else
            textcolor(WHITE);

         strcpy(fname,buffer[i].name);
         fname[24]=0;
         printf("%-25s ",fname);
         totalbytes+=buffer[i].size;
            
         if ( (i+1)%3==0 && (i+1 < totalfiles) ) 
            printf("\n");

      };

      if (style == 1){ //list view style
         if (buffer[i].attb&FILE_MOUNT)
            textcolor(LIGHTBLUE);
         else if (buffer[i].attb&FILE_DIRECTORY)
            textcolor(GREEN);
         else if (buffer[i].attb&FILE_OEXE)
            textcolor(YELLOW);
         else
            textcolor(WHITE);
                    
         strcpy(fname,buffer[i].name);
         fname[24]=0;
         printf("%-25s ",fname);
            
         textcolor(WHITE);
         printf("%10d %14s %14s\n",buffer[i].size,
         vfs_attbstr(&buffer[i],temp), datetostr(&buffer[i].date_modified,
                       mdatestr));
                       
         totalbytes+=buffer[i].size;


         //try to make it fit the screen
         if ((i+1) % 23==0){
            char c;
            printf("Press Q to quit or any other key to continue ...");
            c=getch();
            printf("\n");
            if (c=='q'||c=='Q') 
               break;
         };
      };       
   };
    
   textcolor(WHITE);
   printf("\nTotal Files: %d  Total Size: %d bytes\n", totalfiles, totalbytes);
   free(buffer);
    
};

static void df_find_mount(vfs_node *dir, int devid, char *out)
{
   vfs_node *n;

   if (out[0] || dir == 0 || dir->files == 0 ||
       dir->files == (vfs_node*)VFS_NOT_MOUNTED)
      return;

   for (n = dir->files; n != 0; n = n->next) {
      if ((n->attb & FILE_MOUNT) && n->memid == devid) {
         getpath(n, out);
         return;
      }
      if ((n->attb & FILE_DIRECTORY) && n->files &&
          n->files != (vfs_node*)VFS_NOT_MOUNTED)
         df_find_mount(n, devid, out);
      if (out[0])
         return;
   }
}

void console_iobench(void)
{
   const char *path = "/icsos/apps/tcc.exe";
   DWORD t0, t1, size = 0, i;
   DWORD hits, misses, fills, slots, merged;
   void *buf;
   DWORD cold_ms, warm_ms, mmap_ms;

   printf("iobench: sequential read of %s\n", path);
   blkcache_reset_stats();

   /* Cold read (first pass may still warm FAT/BPB). */
   t0 = getprecisetime();
   buf = vfs_mapfile(path, &size);
   t1 = getprecisetime();
   if (!buf) {
      printf("iobench: FAIL map %s\n", path);
      return;
   }
   cold_ms = t1 - t0;
   free(buf);

   /* Warm read — should hit block cache heavily. */
   t0 = getprecisetime();
   buf = vfs_mapfile(path, &size);
   t1 = getprecisetime();
   if (!buf) {
      printf("iobench: FAIL warm map\n");
      return;
   }
   warm_ms = t1 - t0;
   free(buf);

   /* Third pass through mmap-style path again for stability. */
   t0 = getprecisetime();
   for (i = 0; i < 3; i++) {
      buf = vfs_mapfile(path, &size);
      if (!buf) break;
      free(buf);
   }
   t1 = getprecisetime();
   mmap_ms = (t1 - t0) / (i ? i : 1);

   blkcache_stats(&hits, &misses, &fills, &slots);
   blkcache_mq_stats(&merged);
   printf("iobench: size=%u bytes\n", (unsigned)size);
   printf("iobench: cold_map=%u ms  warm_map=%u ms  avg3=%u ms\n",
          (unsigned)cold_ms, (unsigned)warm_ms, (unsigned)mmap_ms);
   if (warm_ms > 0)
      printf("iobench: speedup=%u.%ux (cold/warm)\n",
             (unsigned)(cold_ms / warm_ms),
             (unsigned)((cold_ms * 10 / warm_ms) % 10));
   else if (cold_ms > warm_ms)
      printf("iobench: warm was <1 timer tick (good)\n");
   printf("iobench: cache hits=%u misses=%u fills=%u slots=%u merged=%u\n",
          (unsigned)hits, (unsigned)misses, (unsigned)fills,
          (unsigned)slots, (unsigned)merged);
   if (hits > 0)
      printf("IOBENCH_CACHE_OK\n");
   if (warm_ms <= cold_ms)
      printf("IOBENCH_PASS\n");
   else
      printf("IOBENCH_WARN warm slower than cold\n");
}

void console_df()
{
   int i;

   printf("%-10s %9s %9s %9s %4s %s\n",
          "Device", "Size(KB)", "Used(KB)", "Free(KB)", "Use%", "Mounted");

   for (i = 0; i < MAXDEVICES; i++) {
      devmgr_block_desc *blk;
      DWORD raw_bytes = 0, total_bytes = 0, free_bytes = 0;
      DWORD size_kb, used_kb, free_kb, pct;
      int have_raw = 0, have_fs = 0;
      char mount[256];
      const char *name;

      if (devmgr_devlist[i] == 0)
         continue;
      if (devmgr_devlist[i]->type != DEVMGR_BLOCK)
         continue;

      blk = (devmgr_block_desc*)devmgr_devlist[i];
      name = blk->hdr.name;

      if (blk->total_blocks && blk->get_block_size) {
         DWORD nblk = (DWORD)bridges_call((devmgr_generic*)blk, &blk->total_blocks);
         DWORD bsz = (DWORD)bridges_call((devmgr_generic*)blk, &blk->get_block_size);
         if (bsz > 0 && bsz != (DWORD)-1 && nblk != (DWORD)-1) {
            raw_bytes = nblk * bsz;
            have_raw = 1;
         }
      }

      /* Probe FAT on any readable  volume. Skip uninitialized floppy
         (FDC waits hang) and CD-ROM (2048-byte sectors). */
      if (blk->read_block) {
         int skip = 0;
         if (name[0] == 'f' && name[1] == 'd' && !devmgr_getlock(i))
            skip = 1;
         if (name[0] == 'c' && name[1] == 'd')
            skip = 1;
         if (strcmp(name, "null") == 0)
            skip = 1;
         if (!skip)
            have_fs = (fat_statfs(i, &total_bytes, &free_bytes) == 0);
      }

      mount[0] = 0;
      if (vfs_root)
         df_find_mount(vfs_root, i, mount);
      if (mount[0] == 0)
         strcpy(mount, "-");

      if (have_fs) {
         size_kb = total_bytes / 1024;
         free_kb = free_bytes / 1024;
         used_kb = (total_bytes - free_bytes) / 1024;
         pct = (total_bytes == 0) ? 0 : (used_kb * 100) / (size_kb ? size_kb : 1);
         printf("%-10s %9u %9u %9u %3u%% %s\n",
                name, size_kb, used_kb, free_kb, pct, mount);
      } else if (have_raw) {
         size_kb = raw_bytes / 1024;
         printf("%-10s %9u %9s %9s %4s %s\n",
                name, size_kb, "-", "-", "-", mount);
      } else {
         printf("%-10s %9s %9s %9s %4s %s\n",
                name, "-", "-", "-", "-", mount);
      }
   }
}

/* ==================================================================
   console_execute(const char *str):
   * This command is used to execute a console string.

*/
int console_execute(const char *str){
   char temp[512];
   char *u;
   int command_length = 0;
   signed char mouse_x, mouse_y, last_mouse_x=0, last_mouse_y=0;
  
   //make a copy so that strtok wouldn't ruin str
   strcpy(temp,str);
   u=strtok(temp," ");
  
   if (u == 0) 
      return;
  
   command_length = strlen(u);    
    
   //check if a pathcut command was executed
   if (u[command_length - 1] == ':'){
      char temp[512];
      sprintf(temp,"cd %s",u);            
      console_execute(temp); 
   }else 
   if (strcmp(u,"fgman") == 0){  //--  Foreground manager
      fg_set_state(1);
   }else 
   if (strcmp(u,"mouse") == 0){  //--  Activate the mouse
      while (!kb_ready()){
         get_mouse_pos(&mouse_x,&mouse_y);
         printf("Mouse (x,y): %d %d\n",mouse_x, mouse_y);
         while ((last_mouse_x == mouse_x) && (last_mouse_y==mouse_y)){
            get_mouse_pos(&mouse_x,&mouse_y);
         }
         last_mouse_x=mouse_x;
         last_mouse_y=mouse_y; 
      }
   }else 
   if (strcmp(u,"shutdown") == 0){  //-- Shuts down the system.
      sendmessage(0,MES_SHUTDOWN,0);
   }else
   if (strcmp(u,"procinfo") == 0){  //-- Show process information. Args: <pid>
      int pid;             
      u=strtok(0," ");
      if (u!=0){
         pid = atoi(u);
         show_process_stat(pid);
      };
   }else
   if (strcmp(u,"meminfo") == 0){   //-- Show memory map information.
      mem_interpretmemory(memory_map,map_length);
   }else
   if (strcmp(u,"pause") == 0){     //-- Waits for a key press
      printf("press any key to continue or 'q' to quit..\n");
      if (getch() == 'q') 
         return -1;
   }else
   if (strcmp(u,"lspcut") == 0){    //-- Shows a list of path aliases. 
      vfs_showpathcuts();
   }else
   if (strcmp(u,"pcut") == 0){      //-- Creates a path alias. Args: <alias:> [path]
      char *u2,*u3;
      u2 = strtok(0," ");
      u3 = strtok(0," ");
      if (u2 != 0){
         if (vfs_addpathcut(u2,u3) == -1){
            printf("Invalid pathcut specified.\n");
         }else{
            printf("Pathcut added.\n"); 
         }                               
      }else{
         printf("Wrong number of parameters specified.\n");
      }
   }else
   if (strcmp(u,"rmdir") == 0){     //-- Removes a directory and all its subdirectories. Args: <dirname>
      char *u2 = strtok(0," ");
      if (u2 != 0){
         char c;                
         printf("*Warning!* This will delete all files and subdirectories!\n");
         printf("Do you wish to continue? (y/n):");
         c = getch();
         if (c == 'y'){
            printf("Please wait..\n");                        
            if (rmdir(u2) != -1)
               printf("Remove directory successful!\n");
            else
               printf("Error while removing directory.\n");
         }else{
            printf("Remove directory cancelled.\n");
         }
      }else{
         printf("Invalid parameter.\n"); 
      }
   }else
   if (strcmp(u,"rempcut") == 0){   //-- Removes a path alias. Args: <alias:>
      char *u2;
      u2 = strtok(0," ");
      if (u2 != 0){
         if (vfs_removepathcut(u2) == -1)
            printf("Invalid Pathcut or pathcut not found\n");
         else
            printf("Pathcut removed.\n");   
      }else{
         printf("Wrong number of parameters specified\n");
      }               
   }else
   if (strcmp(u,"newconsole") == 0){   //-- Creates a new console.  
      //create a new console         
      console_new();
      printf("New console thread created.\n");                   
   }else  
   if (strcmp(u,"ver") == 0) {         //-- Shows version information.
      printf("%s\n",dex32_versionstring);
      printf("%s\n",OS_VERSION);
   }else
   if (strcmp(u,"cpuid") == 0){        //-- Displays CPU information. 
      hardware_cpuinfo mycpu;
      hardware_getcpuinfo(&mycpu);
      hardware_printinfo(&mycpu);
   }else            
   if (strcmp(u,"exit") == 0){         //-- Exits a console session.
      fg_exit();
      exit(0);              
   }else  
   if (strcmp(u,"echo") == 0){         //-- Displays a string. Args: <string>  
      u=strtok(0,"\n");
      if (u!=0)              
         printf("%s\n",u);
   }else  
   if (strcmp(u,"use") == 0){          //-- Tells the extension manager to use the extension: Args: <extension>  
      u=strtok(0," ");
      if (extension_override(devmgr_getdevicebyname(u),0) == -1){
         printf("Unable to install extension %s.\n",u);                
      };            
   }else        
   if (strcmp(u,"off") == 0){          //-- Power off the machine.
      dex32apm_off();
   }else
   if (strcmp(u,"files") == 0){        //-- Shows list of currently open files.
      file_showopenfiles();
   }else
   if (strcmp(u,"find") == 0){         //-- Finds a file.
      u=strtok(0," ");
      if (u != 0)
         findfile(u);
   }else
   if (strcmp(u,"dkill") == 0){         //-- Dirty kill a user process/thread. No cleanup is done. Args: <pid>
      u=strtok(0," ");
      if (u!=0){
         ps_user_kill(atoi(u));
      }
   }else
   if (strcmp(u,"kill") == 0){         //-- Kills a thread/process. Performs cleanup.  Args: <thread name>
      //kernel_file_io_demo();
      u=strtok(0," ");
      if (u!=0){
         dex32_killkthread_name(u);
      }
   }else
   if (strcmp(u,"procs") == 0 || strcmp(u,"ps") == 0){  //-- List the running processes. "ps" can also be used.
      show_process();
   }else
   if (strcmp(u,"cls") == 0 || strcmp(u,"clear") == 0){          //-- Clears the screen. 
      clrscr();
      unsigned char stk[10240];
      //createkthread((void *)runner,"runner",10240);
      //createthread((void*)runner,stk,10240);
   }else
   if (strcmp(u,"help") == 0){         //-- Displays this help screen.
      console_execute("type /icsos/icsos.hlp");
   }else
   if (strcmp(u,"df") == 0){           //-- Shows free space on block devices.
      console_df();
   }else
   if (strcmp(u,"iobench") == 0){      //-- Benchmark file I/O / block cache.
      console_iobench();
   }else
   if (strcmp(u,"umount") == 0){       //-- Unmounts a mounted device. Args: <mount point>
      char *u =strtok(0," ");
      if (u!=0){
         if (vfs_unmount_device(u)==-1)
            printf("umount failed.\n");
         else
            printf("%s umounted.\n",u);
      }else{
         printf("Missing parameter.\n");
      }                    
   }else
   if (strcmp(u,"mount") == 0){        //-- Mounts a device. Args: fat/cdfs <partition/device> <mount point> 
      char *fsname,*devname,*location;
      fsname=strtok(0," ");
      devname=strtok(0," ");
      location=strtok(0," ");
               
      if (vfs_mount_device(fsname, devname, location) == -1)
         printf("mount not successful.\n");
      else
         printf("mount successful.\n");  
         //fat12_mount_root(root,floppy_deviceid);
   }else
   if (strcmp(u,"pwd") == 0){         //-- Shows the current working directory.
      char temp[255];
      printf("%s\n",showpath(temp));
   }else
   if (strcmp(u,"lsmod") == 0){        //-- Shows the list of loaded libraries and modules. 
      showlibinfo();
   }else
   if (strcmp(u,"mem") == 0){          //-- Shows memory information.
      meminfo();
   }else
   if (strcmp(u,"mkdir") == 0){        //-- Creates a directory. Args: <directory name> 
      u=strtok(0," ");
      if (u!=0){
         if (mkdir(u) == -1)
            printf("mkdir failed.\n");
      }
   }else       
   if (strcmp(u,"run") == 0){          //-- Executes a batch file or script. Args: <script>
      u=strtok(0," ");
      if (u!=0){
         if (script_load(u) == -1){
            printf("console: Error loading script file.\n");
         };            
      }
   }else    
   if (strcmp(u,"ls") == 0||strcmp(u,"dir") == 0){ //-- Shows directory listing. Args: [-l | -osize | -oname] 
      int style=0, ordering = 0;
      char v[20];
   
      u=strtok(0," ");
      if (u != 0){
         do {
            strcpy(v,u);
            if (strcmp(v,"-l") == 0) 
               style=1;
            if (strcmp(v,"-oname") == 0) 
               ordering  = 0;
            if (strcmp(v,"-osize") == 0) 
               ordering  = 1;
            u=strtok(0," ");
         } while (u!=0);
      };
      console_ls(style, ordering);
   }else
   if (strcmp(u,"del") == 0){             //-- Deletes a files or directory. Args: <filename/dirname>
      int res;
      u=strtok(0," ");
      if (u!=0){
         char *u3=strtok(0," ");
         if (u3==0){
            delfile(u);
            printf("File deleted.\n");
         }else{
            printf("Invalid parameter.\n");
         }
      }else{
         printf("Missing parameter.\n");
      }
   }else
   if (strcmp(u,"ren") == 0){            //-- Renames a file. Args: <oldname> <newname>
      char *u2,*u3;
      u2=strtok(0," ");
      u3=strtok(0," ");               
      if (u2!=0 && u3!=0){
         if (rename(u2, u3)) 
            printf("File renamed.\n");
         else
            printf("Error renaming file.\n");
      }else{
         printf("Missing parameter.\n"); 
      }   
   }else
   if (strcmp(u,"type") == 0 || strcmp(u,"cat") == 0 ){ //-- Displays the contents of a file. Args: <filename> [-p]
      u=strtok(0," ");
      if (u!=0){
         if (console_showfile(u,0)==-1)
            printf("error opening file.\n");
      }else{
         printf("missing parameter.\n");
      }
   }else
   if (strcmp(u,"copy") == 0 || strcmp(u,"cp") == 0){ //-- Copy source to destination: Args: <source> <destination>
      u=strtok(0," ");
      if (u!=0){
         char *u2 = strtok(0," ");
         if (u2!=0){
            if (fcopy(u,u2) == -1){
               printf("Copy failed. Error while copying.\n");
               printf("Destination directory might not be present.\n");
            };
         };  
      };
   }else              
   if (strcmp(u,"cd") == 0){     //-- Changes working directory. Args: <directory>
      u=strtok(0," ");
      if (u!=0){
         if (!changedirectory(u))
            printf("cd: Cannot find directory\n");
      }else{
         changedirectory(0); //go to working directory
      } 
   }else
   if (strcmp(u,"loadmod") == 0){   //-- Loads a shared library (.dll or .so). Args: <module filename> 
      u=strtok(0," ");
      if (u!=0){
         if (loadDLL(u,str) == -1)
            printf("Unable to load %s.\n",u);
         else
            printf("Load module Successful.\n");  
      }else{
            printf("missing parameter.\n");
      }
   }else
   if (strcmp(u,"lsdev") == 0){  //-- Lists all modules currently installed and available. 
      devmgr_showdevices();
   }else
   if (strcmp(u,"lsext") == 0){  //-- Lists all extensions.
      extension_list();
   }else
   if (strcmp(u,"libinfo") == 0){ //-- Shows library information.
      u=strtok(0," ");
      module_listfxn(u);
   }else
   if (strcmp(u,"time") == 0){   //-- Displays date and time.
      printf("%d/%d/%d %d:%d.%d (%d total seconds since 1970)\n",time_systime.day,
               time_systime.month, time_systime.year,
               time_systime.hour, time_systime.min,
               time_systime.sec,time());
   }else
   if (strcmp(u,"set") == 0){    //-- Sets an environment variable. Args: <key>=<value>
      u=strtok(0," ");
      if (u==0){
         env_showenv();
      }else{
         char *name  = strtok(u,"=");
         char *value = strtok(0,"\n");
         env_setenv(name, value, 1);
      }; 
   }else         
   if (strcmp(u,"unload") == 0){ //-- Unloads a library. Args: <library name>
      u=strtok(0," ");
      if (u!=0){
         if (module_unload_library(u) == -1)
            printf("Error unloading library");
   	};
   }else
   if (strcmp(u,"demo_graphics") == 0){   //-- Runs the graphics demonstration.
      demo_graphics();
   }else
   if (strcmp(u,"cc") == 0){   //-- Builds a C program (invokes tcc.exe). Args: <name.exe> <name.c>
      char src[30],exe[30],cmdline[256],path[256];
      char sdk_home[128]="";
      env_getenv("SDK_HOME",sdk_home);
      env_getenv("PATH",path);
      if ( (strcmp(sdk_home,"")==0) || strcmp(path,"")==0 ){
         printf("Please set the SDK_HOME and PATH environment variables first.\n");
      }else{
         u=strtok(0," ");
         if (u!=0){
            strcpy(exe,u);
            u=strtok(0," ");
            if (u!=0){
               strcpy(src,u);
               sprintf(cmdline,"%s/tcc.exe -nostdlib -static -o%s %s -B%s %s/tccsdk.c %s/posix.c %s/libtcc1.c %s/crt1.c %s/setjmp.c",
                        path,exe,src,sdk_home,sdk_home,sdk_home,sdk_home,sdk_home,sdk_home);
               user_execp("/icsos/apps/tcc.exe",0,cmdline);
            }else{
               printf("Usage: cc <name.exe> <name.c>\n");
            }
         }else{
               printf("Usage: cc <name.exe> <name.c>\n");
         }
      }
   }else
   if (strcmp(u,"reboot") == 0){  //-- Reboot the machine (keyboard + QEMU ports).
      machine_reboot();
   }else
   if (strcmp(u,"kbuild") == 0){  //-- Rebuild the kernel with in-OS tcc.
      kbuild_run(0);
   }else
   if (strcmp(u,"kexec") == 0){  //-- Boot a kernel ELF from ramdisk/path.
      u=strtok(0," ");
      if (!u)
         u="/ramdisk/Kernel64.bin";
      if (kexec_load(u) == 0)
         kexec_reboot();
   }else
   if (strcmp(u,"fullhost") == 0){  //-- tccboot then kbuild with tccnew.
      fullhost_run();
   }else
   if (strcmp(u,"exectest") == 0){  //-- Run the host-built hello.exe (ELF loader smoke test).
      printf("exectest: running /icsos/apps/hello.exe\n");
      if (!user_execp("/icsos/apps/hello.exe", 0, "/icsos/apps/hello.exe"))
         printf("EXEC_TEST_FAIL\n");
      else
         printf("EXEC_TEST_PASS\n");
   }else
   if (strcmp(u,"posixio") == 0){
      printf("posixio: running /icsos/apps/posixio.exe\n");
      if (!user_execp("/icsos/apps/posixio.exe", 0, "/icsos/apps/posixio.exe"))
         printf("POSIXIO_FAIL\n");
   }else
   if (strcmp(u,"spawntest") == 0){
      printf("spawntest: running /icsos/apps/spawn.exe\n");
      if (!user_execp("/icsos/apps/spawn.exe", 0, "/icsos/apps/spawn.exe"))
         printf("SPAWN_FAIL\n");
   }else
   if (strcmp(u,"selfhost") == 0){  //-- Compile a test program with in-OS tcc and run it.
      char cmd[512];
      int ok = 1;
      /* Stage onto ramdisk so compiles do not re-hit ATAPI mid-run. */
      if (ok) {
         printf("selfhost: staging tcc + sources onto /ramdisk\n");
         if (fcopy("/icsos/apps/tcc.exe", "/ramdisk/tcc.exe") == -1 ||
             fcopy("/icsos/apps/min.c", "/ramdisk/min.c") == -1 ||
             fcopy("/icsos/apps/hello.c", "/ramdisk/hello.c") == -1) {
            printf("SELFHOST_TEST_FAIL stage\n");
            ok = 0;
         }
      }
      if (ok) {
         printf("selfhost: compiling /ramdisk/min.c (no includes)\n");
         sprintf(cmd,
                 "/ramdisk/tcc.exe -nostdlib -static -o/ramdisk/min.exe /ramdisk/min.c");
         if (!user_execp("/ramdisk/tcc.exe", 0, cmd)) {
            printf("SELFHOST_TEST_FAIL min.c\n");
            ok = 0;
         }
      }
      if (ok) {
         printf("selfhost: running /ramdisk/min.exe\n");
         if (!user_execp("/ramdisk/min.exe", 0, "/ramdisk/min.exe")) {
            printf("SELFHOST_TEST_FAIL run min.exe\n");
            ok = 0;
         }
      }
      if (ok) {
         printf("selfhost: compiling /ramdisk/hello.c\n");
         sprintf(cmd,
                 "/ramdisk/tcc.exe -nostdlib -static -o/ramdisk/hello.exe /ramdisk/hello.c");
         if (!user_execp("/ramdisk/tcc.exe", 0, cmd)) {
            printf("SELFHOST_TEST_FAIL hello.c\n");
            ok = 0;
         }
      }
      if (ok) {
         printf("selfhost: running /ramdisk/hello.exe\n");
         if (!user_execp("/ramdisk/hello.exe", 0, "/ramdisk/hello.exe")) {
            printf("SELFHOST_TEST_FAIL run hello.exe\n");
            ok = 0;
         }
      }
      if (ok)
         printf("SELFHOST_TEST_PASS\n");
   }else
   if (strcmp(u,"tccboot") == 0){  //-- Rebuild TinyCC with the in-OS tcc.
      tccboot_run();
   }else
   if (u[0] == '$'){                      //-- Sends message to a device.
      int i, devid;
      char devicename[255],*cmd;

      for (i=0;i<20 && u[i+1];i++){
         devicename[i] = u[i+1];
      };
      devicename[i] = 0;
      printf("Sending command to %s\n",devicename);
      devid = devmgr_finddevice(devicename);
               
      if (devid != -1){
         if (devmgr_sendmessage(devid,DEVMGR_MESSAGESTR,str)==-1)
            printf("console: send_message() failed or not supported.\n");
      }else{
         printf("console: cannot find device.\n");
      }   

   }else{         //ok it is not a command, maybe it's an executable?
      if (u!=0){
         char path[256]="", tmp[256];
         env_getenv("PATH",path);     
         if (strcmp(path,"")==0){
            strcpy(path,"/icsos/apps");
            sprintf(tmp,"%s/%s",path,u);
            if (!user_execp(tmp, 0, str)){
               printf("Command or executable not found.\n");
            }
         }else{
            sprintf(tmp,"%s/%s",path,u);
            if (!user_execp(tmp, 0, str)){
               printf("Command or executable not found.\n");
            }
         }
      }
   }
   //normal termination
   return 1;
};

int console_new(){
   //create a new console         
   char consolename[255];
   sprintf(consolename,"console(%d)", console_first);    
   return createkthread((void*)console, consolename, 200000);
};

void console_main(){
   DEX32_DDL_INFO *myddl=0;
   fg_processinfo *myfg;
   char s[256]="";
   char temp[256]="";
   char last[256]="";
   char console_fmt[256]="%cdir% %% ";
   char console_prompt[256]="cmd >";
    
   DWORD ptr;
    
   myddl =Dex32CreateDDL();    

    
   Dex32SetProcessDDL(myddl, getprocessid());
   myfg = fg_register(myddl, getprocessid());
   fg_setforeground( myfg->id );
   {
      tty_t *t = tty_alloc((struct _dex32_direct_device_hdl *)myddl,
                           TTY_ECHO | TTY_ICANON | TTY_ISIG);
      tty_set_fg(t);
      tty_attach_proc(current_process, t);
      if (myfg && myfg != (fg_processinfo *)-1)
         myfg->tty = t;
   }

   clrscr();
   printf("console: tmux keys  C-b c/n/p/l/0-9/w/x/?  (F2 new, F12 next)\n");
   strcpy(last,"");
    
   if (console_first == 0) {
      if (kernel_kexeced && strcmp(kernel_cmdline, "kexeced") == 0) {
         printf("KEXEC_BOOT_OK\n");
         serial_puts("KEXEC_BOOT_OK\n");
         machine_reboot();
      } else
         script_load("/icsos/autoexec.bat");
   }
    
   console_first++;
   /* Prefer userland sh.exe when present (PATH on CD / ramdisk). */
   if (user_execp("/icsos/apps/sh.exe", 0, "/icsos/apps/sh.exe")) {
      fg_exit();
      exit(0);
      return;
   }
   do{
      textcolor(WHITE);
      textbackground(BLACK);
      prompt_parser(console_fmt,console_prompt);
    
      textcolor(LIGHTBLUE);
      printf("%s",console_prompt);
      textcolor(WHITE);
    
      if (strcmp(s,"@@")!=0 && strcmp(s,"!!")!=0)
         strcpy(last,s);
    
      getstring(s, myddl);
   
      if (strcmp(s,"!")==0){
         sendtokeyb(last,&_q);
      }
      else if (strcmp(s,"!!")==0){
         sendtokeyb(last,&_q);
         sendtokeyb("\r",&_q);
      }
      else   
         console_execute(s);
   } while (1);
};

