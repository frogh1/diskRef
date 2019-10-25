//gcc -framework DiskArbitration -framework CoreFoundation diskRef.c -o diskRef

#include <stdio.h>

#include <CoreFoundation/CoreFoundation.h>
#include <DiskArbitration/DADisk.h>
#include <DiskArbitration/DADissenter.h>
#include <DiskArbitration/DASession.h>
#include <DiskArbitration/DiskArbitration.h>

int printDictionaryAsXML(CFDictionaryRef dict)
{
    CFStringRef failureReason;
    CFErrorRef error = NULL;

    CFDataRef xml = CFPropertyListCreateData(kCFAllocatorDefault,dict,kCFPropertyListXMLFormat_v1_0,0,&error);
    if (!xml && error) 
    {
        failureReason = CFErrorCopyFailureReason(error);
        printf("Failed to get XML error: %s\n",CFStringGetCStringPtr(failureReason,kCFStringEncodingASCII));
        if(failureReason) CFRelease(failureReason);
        CFRelease(error);
        return -1;
    }

    write(STDOUT_FILENO, CFDataGetBytePtr(xml), CFDataGetLength(xml));
    CFRelease(xml);

    return 0;
}
void handledisk(DADiskRef disk)
{
    const char *name =  DADiskGetBSDName(disk);
    printf("%s %s\n", __FUNCTION__,name);
}

void diskAppearedCallback(DADiskRef disk, void *context)
{
    CFDictionaryRef description = NULL;
    const void *ref;
    char deviceType[64] = {0};
    unsigned char isWritable;
    description = DADiskCopyDescription(disk);
    if (description == NULL) {
        printf("disk description is null\n");
        goto done;
    }

    //CFShow(description);
    printDictionaryAsXML(description);

    const char  *diskname =  DADiskGetBSDName(disk);

    if (CFDictionaryContainsKey(description,CFSTR("DADeviceProtocol")))
    {
        ref = CFDictionaryGetValue(description, CFSTR("DADeviceProtocol"));
        if (!ref)
        {
            printf("get DADeviceProtocol value error\n");
            goto done;
        }
        CFStringGetCString(ref,deviceType,sizeof(deviceType),kCFStringEncodingASCII);
        printf("disk: %s type:%s\n", diskname, deviceType);
        if (0 != CFStringCompare(ref, CFSTR("USB"), 0 ))
        {
            goto done;
        }
        
        ref = CFDictionaryGetValue(description, kDADiskDescriptionMediaWritableKey);
        if(!ref)
        {
            goto done; 
        }
        isWritable = CFBooleanGetValue(ref);
        if (isWritable)
        {
            handledisk(disk);
        }
    }

  done:  
    if(description) 
        CFRelease(description);

}
void diskDisappearedCallback(DADiskRef disk, void *context)
{
    printf("disk %s disappeared\n", DADiskGetBSDName(disk));
 }


void diskRef()
{
    DASessionRef session;
    session = DASessionCreate(kCFAllocatorDefault);
    if (!session)
    {
        printf("session create error\n");
        return;
    }
    DARegisterDiskAppearedCallback(session, NULL, diskAppearedCallback, NULL);
    DARegisterDiskDisappearedCallback(session, NULL, diskDisappearedCallback, NULL);

    DASessionScheduleWithRunLoop(session,
                                CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    /* Start the run loop.  (Don't do this if you already have
   a running Core Foundation or Cocoa run loop.) */
    CFRunLoopRun();

    /* Clean up a session. */
    DASessionUnscheduleFromRunLoop(session,
        CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    CFRelease(session);
}


int main(int argc,char *argv[])
{
    diskRef();
    return 0;
}
