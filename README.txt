server.c contains Server code
    gcc server.c -o server -luuid
    ./server 2000

    ******************** here port number is assignned: 2000    ***************************

client.c contains client code
    gcc client.c -o client
    ./client 127.0.0.1 2000
    *******************   here Ip address of server: 127.0.0.1     port no: 2000   ******************************

TASK 1:

/active:  
        It will give you list of active clients

/logout:
        It will disconnect the client from server.
        Press 2 enter you will exit from client code.

/send dest_client_uuid  message:
        It will send the message to dest_client_uuid from the current_client 

/send all message:
        it will send the the message to all other clients

/chatbot login
        You will enter into FAQ

Simply type que to get answer inside chatbot login.

/chatbot logout
        To exit from FAQ chatbot and return you to normal texting application.

/history dest_uuid:
        It will return you all the msg sent and received between sender and dest_uuid

/history_delete dest_uuid:
        It will delete the msg sent to dest_uuid from client_uuid.

/delete all:
        it will delete all the msg.


/chatbot_v2 login
        It will enter you in updated bot i.e gpt2_bot

/chatbot_v2 logout
        It will exit you from updated bot i.e. gpt2_bot


        NOTE: IT WILL TAKE SOME TIME TO GENERATE RESPONSE FOR UPDATED GPT2 RESPONSE, PLEASE WAIT FOR SOMETIME TO GET RESPONSE.
        ALL THE RESPONSES AND PROMPT ARE ALSO STORED IN "shared_memory.txt" that works as shared memory


exit:
        To exit from client code directly.



Updated bot responses are captured using *******************    gpt_2_gen.py        ***************
                    python3 gpt_2_gen.py




All conversation  of text between clients is maintained in conversation_log.txt



LOG_FILE:
You will get all the msg record in                  "conversation_log.txt"
Shared Memory work is performed by                  "shared_memory.txt"