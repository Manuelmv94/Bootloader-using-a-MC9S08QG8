infile=open('LED1.s19','r')
outfile=open('process_s19.txt',"wb")
line_index=0
numberlines=0
for line in infile:
    numberlines+=1
print(bytes([numberlines]))
infile.close()
outfile.write(bytes([numberlines]))

infile=open('LED1.s19','r')
for line in infile:
    if line_index>0:
        char_index=0
        message_size=""
        address=""
        message=""
        count=0;
        messagef=""
        messagef2=[];
        s1=False
        for ch in line:
            if char_index==1 and ch=='1':
                s1=True
            if s1==True:
                if char_index>1 and char_index<4:
                    message_size+=ch
                    message+=ch
                    count+=1
                    if count==2:
                        messagef+=" "+str(int(message,16))
                        messagef2.append(int(message,16))
                        message=""
                        count=0
                elif char_index>3 and char_index<(int(message_size,16)*2+2):
                    message+=ch
                    count+=1
                    if count==2:
                        messagef+=" "+str(int(message,16))
                        messagef2.append(int(message,16))
                        message=""
                        count=0




            char_index+=1
        if s1==True:
            print (messagef2)
            newFileByteArray = bytes(messagef2)
            #print(newFileByteArray)
            outfile.write(newFileByteArray)
    line_index+=1

infile.close()
outfile.close()
