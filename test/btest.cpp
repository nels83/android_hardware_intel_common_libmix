#include "IntelMetadataBuffer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SUCCESS "PASS IntelMetadataBuffer Unit Test\n"
#define FAIL "Fail IntelMetadataBuffer Unit Test\n"

int main(int argc, char* argv[])
{
    IntelMetadataBuffer *mb1, *mb2;
    uint8_t* bytes;
    uint32_t size;
    IMB_Result ret;

    MetadataBufferType t1 = MetadataBufferTypeCameraSource;
    MetadataBufferType t2;
    int32_t v1 = 0x00000010;
    int32_t v2 = 0;
    ValueInfo vi1, *vi2 = NULL;
    int32_t ev1[10];
    int32_t *ev2 = NULL;
    unsigned int count;

    if (argc > 1)
        t1 = (MetadataBufferType) atoi(argv[1]);

    if (argc > 2)
        v1 = atoi(argv[2]);

    memset(&vi1, 0, sizeof(ValueInfo));
    
    mb1 = new IntelMetadataBuffer();
    ret = mb1->SetType(t1);
    ret = mb1->SetValue(v1);
    mb1->GetMaxBufferSize();
    if (t1 != MetadataBufferTypeGrallocSource) {
        ret = mb1->SetValueInfo(&vi1);
        ret = mb1->SetExtraValues(ev1, 10);
        ret = mb1->SetExtraValues(ev1, 10);
    }
//  ret = mb1->GetBytes(bytes, size);
    ret = mb1->Serialize(bytes, size);
    printf("assembling IntelMetadataBuffer %s, ret = %d\n", (ret == IMB_SUCCESS)?"Success":"Fail", ret );    

    printf("size = %d, bytes = ", size);
    for(int i=0; i<size; i++)
    {
        printf("%02x ", bytes[i]);
    }
    printf("\n");

    mb2 = new IntelMetadataBuffer();
//  ret = mb2->SetBytes(bytes, size);
    ret = mb2->UnSerialize(bytes, size);
    printf("parsing IntelMetadataBuffer %s, ret = %d\n", (ret == IMB_SUCCESS)?"Success":"Fail", ret );    
    
    ret = mb2->GetType(t2);
    ret = mb2->GetValue(v2);
    ret = mb2->GetValueInfo(vi2);
    ret = mb1->SetExtraValues(ev1, 10);
    ret = mb2->GetExtraValues(ev2, count);

    IntelMetadataBuffer mb3;;
//  mb3 = new IntelMetadataBuffer();
    ret = mb3.SetType(t1);
    ret = mb3.SetValue(v1);
    ret = mb3.SetExtraValues(ev1, 10);
    ret = mb3.SetValueInfo(&vi1);
    ret = mb3.UnSerialize(bytes, size);

    IntelMetadataBuffer *mb4 = new IntelMetadataBuffer(mb3);
    IntelMetadataBuffer *mb5;
    mb5 = mb4;

    printf("t2=%d, v2=%d, vi2=%x, ev2=%x\n", t2, v2, vi2, ev2);
    if (v1 == v2 && t1 == t2 ) {
       if (vi2) {
            if (memcmp(&vi1, vi2, sizeof(ValueInfo)) == 0) {
                if (ev2) {
                   if (memcmp(ev1, ev2, count) == 0)
                       printf(SUCCESS);
                   else
                       printf(FAIL);
                }else
                    printf(SUCCESS);
            }else
                printf(FAIL);
       }else
           printf(SUCCESS);
    }else
        printf(SUCCESS);

    return 1;
}
