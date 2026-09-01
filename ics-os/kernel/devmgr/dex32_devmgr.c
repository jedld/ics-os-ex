/*
  Name: DEX device manager
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 10/02/04 20:50
  Description: The device manager is the primary module for handling the device
  in the system. A device in DEX may also include the scheduler, Virtual Memory Manager
  and other operating system services.
  
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
*/
#include "dex32_devmgr.h"

devmgr_generic **devmgr_devlist;         //points to the list of devices
devmgr_status *devmgr_statuslist;
sync_sharedvar devmgr_busy;

static int devmgr_finddevice_locked(const char *name)
{
    int i;
    for (i=0;i<MAXDEVICES;i++)
        if (devmgr_devlist[i]!=0 &&
                devmgr_statuslist[i].state==DEVMGR_STATE_LIVE &&
                strcmp(name,devmgr_devlist[i]->name)==0)
            return i;
    return -1;
}

static devmgr_generic *devmgr_retire_locked(int deviceid)
{
    devmgr_generic *retired;
    if (!devmgr_lifecycle_can_retire(devmgr_statuslist[deviceid].state,
                                                                     devmgr_statuslist[deviceid].refs))
        return 0;
    retired=devmgr_devlist[deviceid];
    devmgr_devlist[deviceid]=0;
    devmgr_statuslist[deviceid].state=DEVMGR_STATE_DEAD;
    return retired;
}

void devmgr_init()
{
  int i;
  devmgr_interface myinterface;
  devmgr_fs_desc devfs;
  devmgr_stdlib stdlib;
  
  devmgr_devlist=(devmgr_fs_desc**)malloc(MAXDEVICES*sizeof(devmgr_fs_desc*));
  devmgr_statuslist=(devmgr_status*)malloc(MAXDEVICES*sizeof(devmgr_status));
  memset(devmgr_devlist,0,MAXDEVICES*sizeof(devmgr_fs_desc*));
  memset(devmgr_statuslist,0,MAXDEVICES*sizeof(devmgr_status));
  //initialize all devices
  for (i=0;i<MAXDEVICES;i++)
    {
    devmgr_devlist[i]=(devmgr_fs_desc*)0;
    devmgr_statuslist[i].locked = 0;
    devmgr_statuslist[i].refs = 0;
    devmgr_statuslist[i].generation = 0;
    devmgr_statuslist[i].state = DEVMGR_STATE_FREE;
    };

  //the myinterface structure will be passed on to drivers upon startup
  myinterface.hdr.size = sizeof(devmgr_interface);
  myinterface.hdr.type = DEVMGR_SERVICE;
  strcpy(myinterface.hdr.name,"devmgr");
  strcpy(myinterface.hdr.description,"Device Manager Interface");

  //fill up all the required information
  myinterface.devmgr_register = devmgr_register;
  myinterface.devmgr_init = devmgr_init;
  myinterface.devmgr_flushblocks = devmgr_flushblocks;
  myinterface.devmgr_identify = devmgr_identify;
  myinterface.devmgr_finddevice = devmgr_finddevice;
  myinterface.devmgr_getdevice = devmgr_getdevice;
    myinterface.devmgr_getdevice_ref = devmgr_getdevice_ref;
    myinterface.devmgr_claimdevice_ref = devmgr_claimdevice_ref;
    myinterface.devmgr_putdevice = devmgr_putdevice;
    myinterface.devmgr_removedevice = devmgr_removedevice;
  myinterface.devmgr_copyinterface = devmgr_copyinterface;
  myinterface.devmgr_disableints = stopints;
  myinterface.devmgr_enableints= startints;
  //register myself
  devmgr_register((devmgr_generic*)&myinterface);

  //create a services interface for the use of drivers
  stdlib.hdr.size = sizeof(devmgr_stdlib);
  stdlib.hdr.type = DEVMGR_STDLIB;
  strcpy(stdlib.hdr.name,"stdlib");
  strcpy(stdlib.hdr.description,"Standard Library Interface");
  stdlib.malloc  = malloc;
  stdlib.printf  = printf;
  stdlib.free    = free;
  stdlib.realloc = realloc;


  //Register standard library
  devmgr_register((devmgr_generic*)&stdlib);
  
  //initialize the device manager's critical section variable
  memset(&devmgr_busy,0,sizeof(devmgr_busy));
};


int devmgr_removedevice(int deviceid)
{
    devmgr_generic *retired=0;
    int retval=-1;
    sync_entercrit(&devmgr_busy);
    if (deviceid>=0 && deviceid<MAXDEVICES &&
        devmgr_devlist[deviceid]!=0 && !devmgr_statuslist[deviceid].locked) {
        if (devmgr_statuslist[deviceid].state==DEVMGR_STATE_LIVE)
            devmgr_statuslist[deviceid].state=DEVMGR_STATE_QUIESCING;
        retired=devmgr_retire_locked(deviceid);
        retval=retired ? 1 : 0;
    }
    sync_leavecrit(&devmgr_busy);
    /* Legacy devmgr_getdevice() callers cannot publish a read-side reference.
       Keep the detached copy allocated until all callers use get/put; no new
       lookup can reach it and referenced users have already drained. */
    (void)retired;
    return retval;
};

//Gets the name of a device
char *devmgr_getname(int deviceid)
{
devmgr_generic *device = devmgr_getdevice(deviceid);
if (device == -1) return "unknown";
return device->name;
};

//gets the device manager interface given the deviceid
devmgr_generic *devmgr_getdevice(int deviceid)
{
    devmgr_generic *retval;
    if (deviceid==-1) return (devmgr_generic*)-1;
    
    //wait until the device manager is ready
    sync_entercrit(&devmgr_busy);
    
    if (deviceid>=0 && deviceid<MAXDEVICES)
        retval = devmgr_devlist[deviceid];
    else
        retval = (devmgr_generic*) -1;
    
    sync_leavecrit(&devmgr_busy);
    
    return retval;
};

/* Referenced lookup for any code that calls through a device operation table.
   Quiescing prevents new references; final put retires a pending removal. */
devmgr_generic *devmgr_getdevice_ref(int deviceid)
{
    devmgr_generic *retval=(devmgr_generic*)-1;
    sync_entercrit(&devmgr_busy);
    if (deviceid>=0 && deviceid<MAXDEVICES &&
        devmgr_devlist[deviceid]!=0 &&
        devmgr_lifecycle_get(devmgr_statuslist[deviceid].state,
                             &devmgr_statuslist[deviceid].refs))
        retval=devmgr_devlist[deviceid];
    sync_leavecrit(&devmgr_busy);
    return retval;
}

/* Atomically acquire both an operation reference and the legacy exclusive
   claim used by mounts. */
devmgr_generic *devmgr_claimdevice_ref(int deviceid)
{
    devmgr_generic *retval=(devmgr_generic*)-1;
    sync_entercrit(&devmgr_busy);
    if (deviceid>=0 && deviceid<MAXDEVICES &&
        devmgr_devlist[deviceid]!=0 &&
        !devmgr_statuslist[deviceid].locked &&
        devmgr_lifecycle_get(devmgr_statuslist[deviceid].state,
                             &devmgr_statuslist[deviceid].refs)) {
        devmgr_statuslist[deviceid].locked=1;
        retval=devmgr_devlist[deviceid];
    }
    sync_leavecrit(&devmgr_busy);
    return retval;
}

void devmgr_putdevice(devmgr_generic *device)
{
    devmgr_generic *retired=0;
    int deviceid;
    if (!device || device==(devmgr_generic*)-1)
        return;
    deviceid=device->id;
    sync_entercrit(&devmgr_busy);
    if (deviceid>=0 && deviceid<MAXDEVICES &&
        devmgr_devlist[deviceid]==device) {
        if (!devmgr_lifecycle_put(&devmgr_statuslist[deviceid].refs))
            printf("devmgr: reference underflow id=%d\n",deviceid);
        retired=devmgr_retire_locked(deviceid);
    }
    sync_leavecrit(&devmgr_busy);
    (void)retired; /* See compatibility reclamation note above. */
}




//used to send a message to a module
int devmgr_sendmessage(int deviceid,int type,DWORD message)
{
    devmgr_generic *dev = devmgr_getdevice_ref(deviceid);
    DWORD cpuflags;
    int retval = 0;
    
    //cannot find device?
    if (dev == -1) return -1;
    
    storeflags(&cpuflags);
    stopints();
    
    //send the message to the device if it supports getting messages
    if (dev->sendmessage!=0)
         retval = bridges_call(dev,&dev->sendmessage,type,message);
    else
         retval = -1;   
    
    restoreflags(cpuflags);
    devmgr_putdevice(dev);
    
    return retval;
};

/*****************************************************************************
devmgr_register - 
Registers a device to the device manager, a device driver is usually loaded
through DLLs (Dynamic Link Libraries) which may be created using various
WIN32 compilers*/
int devmgr_register(devmgr_generic *d)
{
char temp[255];
int devno,retval = -1;


//the size cannot be smaller than devmgr_generic !!
if (d->size < sizeof(devmgr_generic) ) return -1;

//wait until the device manager is ready
sync_entercrit(&devmgr_busy);

//Check atomically for another device with the same name.
if (devmgr_finddevice_locked(d->name)!=-1) {
    sync_leavecrit(&devmgr_busy);
    return -1;
}

devmgr_generic *mydev = (devmgr_generic*)malloc(d->size);
if (!mydev) {
    sync_leavecrit(&devmgr_busy);
    return -1;
}
memcpy(mydev,d,d->size);

for (devno=0;devno<MAXDEVICES;devno++)
    {
        if (devmgr_devlist[devno]==0)
                {
                    mydev->id = devno;
                    devmgr_devlist[devno]=mydev;
                    devmgr_statuslist[devno].refs=0;
                    devmgr_statuslist[devno].locked=0;
                    devmgr_statuslist[devno].generation++;
                    devmgr_statuslist[devno].state=DEVMGR_STATE_LIVE;
                    retval = mydev->id;
                    break;
                };
    };

sync_leavecrit(&devmgr_busy);

if (retval==-1)
    free(mydev);

return retval;
};

//returns the string identifying the type of device
char *devmgr_identify(int type,char *buf)
{
            if (type == DEVMGR_UNKNOWN)
                        strcpy(buf,"Unknown");
                        else
            if (type == DEVMGR_FS)
                        strcpy(buf,"Filesystem");
                        else
            if (type == DEVMGR_DEV)
                        strcpy(buf,"Device Driver");
                        else
            if (type == DEVMGR_EXT)
                        strcpy(buf,"Extension");
                        else
            if (type == DEVMGR_BLOCK)
                        strcpy(buf,"Block device");
                        else
            if (type == DEVMGR_SERVICE || type == DEVMGR_STDLIB)
                        strcpy(buf,"Kernel Service");
                        else
            if (type == DEVMGR_PORTMGR)
                        strcpy(buf,"port manager");
                        else
            if (type == DEVMGR_IOMGR)
                        strcpy(buf,"I/O manager");
                        else
            if (type == DEVMGR_MEM)
                        strcpy(buf,"memory manager");
                        else
            if (type == DEVMGR_PROCESSMGR)
                        strcpy(buf,"process manager");
                        else
            if (type == DEVMGR_CHAR)
                        strcpy(buf,"character device");
                        else
            if (type >= 1000)
            				strcpy(buf,"Kernel Extension");
                        else
                        strcpy(buf,"Unknown");
return buf;
};

int devmgr_getcontext()
{
 return current_process->context;
};

void devmgr_setcontext(int deviceid)
{
   current_process->context = deviceid;
};

void devmgr_setfunction(int fxn)
{
   current_process->function = fxn; 
};

int devmgr_getfunction()
{
  return current_process->function; 
};



void devmgr_showdevices()
{
    int i, num_list = 0;

    //wait until the device manager is ready
    sync_entercrit(&devmgr_busy);
    printf("%-5s %-15s %-15s %-30s\n","ID","Device Name","Type","Description");
    for (i=1;i<MAXDEVICES;i++)
      {
            if (devmgr_devlist[i]!=0)
            {
                char typestr[40];
                strcpy(typestr,devmgr_identify(devmgr_devlist[i]->type,typestr));
                
                if (devmgr_statuslist[i].locked)
                    textcolor(RED);
                else
                    textcolor(WHITE);
                    
                printf("%-5d %-15s %-15s %-30s\n",devmgr_devlist[i]->id,
                                  devmgr_devlist[i]->name,typestr,
                                  devmgr_devlist[i]->description);
                                  
                //try to make it fit the screen
                if ((num_list+1) % 23==0) 
                {
                    char c;
                    printf("Press Q to quit or any other key to continue ...");
                    c=getch();
                    printf("\n");
                    if (c=='q'||c=='Q') break;
                };
                
                num_list++;
            };
      };
    textcolor(WHITE);  
   sync_leavecrit(&devmgr_busy);
};

/*****************************************************************************
devmgr_copyinterface - 
copies the interface of name to interface, safer than getdevice
since any modification to interface will not affect the source interface*/
int devmgr_copyinterface(const char *name,devmgr_generic *interface)
{
int deviceid = devmgr_finddevice(name);

//device not found!
if (deviceid == -1) return -1;

devmgr_generic *srcdevice = devmgr_getdevice_ref(deviceid);
if (srcdevice==(devmgr_generic*)-1) return -1;

//take note of the size to maintain upward and downward compatibility

    if (interface->size < srcdevice->size)
                    memcpy(interface,srcdevice,interface->size);
              else
                    memcpy(interface,srcdevice,srcdevice->size);
                    
devmgr_putdevice(srcdevice);

return 1;
};

/******************************************************************************
devmgr_setlock(devichandle) - sets the lock status of a particular device. This
locking mechanism is not really enforced by the device manager so modules
must still manually check if a device is locked or not using devmgr_getlock()*/
int devmgr_setlock(int devicehandle,int val)
{
    int retval=-1;
    sync_entercrit(&devmgr_busy);
    if (devicehandle>=0 && devicehandle<MAXDEVICES &&
        devmgr_devlist[devicehandle]!=0) {
          devmgr_statuslist[devicehandle].locked = val;
          retval=1;
    }
    sync_leavecrit(&devmgr_busy);
    return retval;
};

/*****************************************************************************
devmgr_getlock(int devicehandle) - returns the lock status of a current
device, if it is locked it returns the value placed in locked*/
int devmgr_getlock(int devicehandle)
{
    int retval=-1;
    sync_entercrit(&devmgr_busy);
    if (devicehandle>=0 && devicehandle<MAXDEVICES &&
        devmgr_devlist[devicehandle]!=0)
        retval=devmgr_statuslist[devicehandle].locked;
    sync_leavecrit(&devmgr_busy);
    return retval;
};

DWORD devmgr_get_generation(int devicehandle)
{
    DWORD generation=0;
    sync_entercrit(&devmgr_busy);
    if (devicehandle>=0 && devicehandle<MAXDEVICES &&
        devmgr_devlist[devicehandle]!=0)
        generation=devmgr_statuslist[devicehandle].generation;
    sync_leavecrit(&devmgr_busy);
    return generation;
}

//returns the device ID of a device given the name
int devmgr_finddevice(const char *name)
{
    int retval;
    //wait until the device manager is ready
    sync_entercrit(&devmgr_busy);

     retval=devmgr_finddevice_locked(name);
    sync_leavecrit(&devmgr_busy);
     return retval;
};

/********************************************************************************
devmgr_tweak_by_ID()
*/
void *devmgr_tweak_by_ID(int deviceid, int function_index, void *new_function)
{
    void **ptr,*ret;
    devmgr_generic *mydev = devmgr_getdevice(deviceid);
    
    if (mydev->type >= 0x1000) function_index += 2;
    
    ptr = (void**) ( mydev + sizeof(devmgr_generic) + 
                     function_index*sizeof(DWORD) );

    ret = *ptr;
    *ptr = new_function;
    return ret;
};

/********************************************************************************
devmgr_tweak_by_addr()
*/
void *devmgr_tweak_by_addr(devmgr_generic *mydev, void **target_function, void *new_function)
{
    void *ret;
    ret = *target_function;
    *target_function = new_function;
    return ret;
};

/********************************************************************************
devmgr_getdevicebyname - returns a pointer to the device interface given a name*/
devmgr_generic *devmgr_getdevicebyname(const char *name)
{
  return devmgr_getdevice(devmgr_finddevice(name));
};

/******************************************************************************
devmgr_flushblocks() - called by the current IO manager to flush the contents
    of all the block devices in the list, if the block device supports it*/
int devmgr_flushblocks()
{
        devmgr_generic *snapshot[MAXDEVICES];
        int i,n=0,err=0;

        /* Snapshot referenced block devices, then invoke drivers without the
             registry lock. Removal quiesces until every snapshot reference drops. */
    sync_entercrit(&devmgr_busy);
    for (i=0;i<MAXDEVICES;i++)
            if (devmgr_devlist[i]!=0 &&
                    devmgr_devlist[i]->type==DEVMGR_BLOCK &&
                    devmgr_lifecycle_get(devmgr_statuslist[i].state,
                                                             &devmgr_statuslist[i].refs))
                snapshot[n++]=devmgr_devlist[i];
    sync_leavecrit(&devmgr_busy);

        for (i=0;i<n;i++) {
            devmgr_block_desc *myblock=(devmgr_block_desc*)snapshot[i];
            int result=0;
            devmgr_setcontext(myblock->hdr.id);
            if (myblock->flush_device!=0)
                result=myblock->flush_device();
            if (result<0 && err==0)
                err=result;
            devmgr_putdevice(snapshot[i]);
        }
    return err;
};

//performs I/O operations to various devices
int devmgr_ioctl(int deviceid, int func , void *buffer,int block,int bufsize)
{
devmgr_generic *buf=(devmgr_generic*)buffer;
if (buf->type == DEVMGR_BLOCK && func == DEVMGR_IOCTL_GETINFO)
  {
    devmgr_generic *buf=(devmgr_generic*)buffer;
    devmgr_block_desc *myblock=(devmgr_block_desc*)devmgr_getdevice_ref(deviceid);
      devmgr_block_info *myinfo=(devmgr_block_info*)malloc(sizeof(devmgr_block_info));
      int size;
      
    if (myblock!=(devmgr_block_desc*)-1 && myinfo)
         {
         //make sure it is really a block device
         if (myblock->hdr.type == DEVMGR_BLOCK)
             {
         	 //set up the correct size of the structure
             //This is for upward and downward compatibility
             if (bufsize<sizeof(devmgr_block_info))
                        size= buf->size;
             else
             			size= sizeof(devmgr_block_info);
             myinfo->hdr.size =size;
             myinfo->hdr.type = DEVMGR_BLOCK_INFO;
             strcpy(myinfo->hdr.name,myblock->hdr.name);
             strcpy(myinfo->hdr.description,myblock->hdr.description);

             //fill in the information
             if (myblock->get_block_size!=0)
             myinfo->blocksize=myblock->get_block_size();
             if (myblock->total_blocks!=0)
             myinfo->maxblocks = myblock->total_blocks();

             memcpy(buffer,myinfo,size);
				 free(myinfo);
				 devmgr_putdevice((devmgr_generic*)myblock);
				 return 1;
			}
             else
                        {
                            free(myinfo);
                            devmgr_putdevice((devmgr_generic*)myblock);
                            return -1;
                        }
         };
         free(myinfo);
         devmgr_putdevice((devmgr_generic*)myblock);
     return -1;
  };

if (func == DEVMGR_IOCTL_READ || func == DEVMGR_IOCTL_WRITE)
 {
 		//validate the deviceid
    devmgr_block_desc *myblock = (devmgr_block_desc*)devmgr_getdevice_ref(deviceid);
    if (myblock!=(devmgr_block_desc*)-1)
      {
      	if (myblock->hdr.type == DEVMGR_BLOCK)
         	{
            	if (myblock->read_block!=0 && func == DEVMGR_IOCTL_READ)
                   {
                       //send an I/O request to the IO manager
                       //read current data
                       char *temp=malloc(bufsize * 512);
                       DWORD hdl=dex32_requestIO(deviceid,IO_READ,block,bufsize,temp);
						     while (!dex32_IOcomplete(hdl));
					         dex32_closeIO(hdl);
							 memcpy(buffer,temp,512);
                       free(temp);	
                               devmgr_putdevice((devmgr_generic*)myblock);
                       return 1;
                   }
               else
               if (myblock->write_block!=0 && func == DEVMGR_IOCTL_WRITE)
               	{
                  	   //send an I/O request to the IO manager
                        //write current data
                        char *temp=malloc(bufsize * 512);
                        memcpy(temp,buffer,512);
                        DWORD hdl=dex32_requestIO(deviceid,IO_WRITE,block,bufsize,temp);
   	   			        while (!dex32_IOcomplete(hdl));
                        dex32_closeIO(hdl);
                        free(temp);
                        devmgr_putdevice((devmgr_generic*)myblock);
                        return 1;

                  };
                             devmgr_putdevice((devmgr_generic*)myblock);
                             return -1;
            }
              else
                        {
                            devmgr_putdevice((devmgr_generic*)myblock);
                            return -1;
                        }
      };
   return -1;
 };



 return 1;
};
