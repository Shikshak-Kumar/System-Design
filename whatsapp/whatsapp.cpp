#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <cassert>
#include <ctime>

using namespace std;

// Enums

enum class MessageStatusType{
    SENT,
    DELIVERED,
    READ
};

enum class NotificationType {
    MESSAGE,
    GROUP
};

enum class MediaType{
    IMAGE,
    AUDIO,
    VEDIO,
    DOCUMENT
};


// USER 

class User {
    private:
        int userId;
        string name;
        string profilePicture;
        string lastseen;
        string status;
        
    public:
        User(int userId, string &name):
        userId(userId),
        name(name),
        profilePicture(""),
        lastseen(""),
        status("") {}
        
        int & getUserId(){
            return userId;
        }
        
        string & getName(){       
            return name;
        }
        
        string & getProfilePicture(){
            return profilePicture;
        }

        string & getStatus(){
            return status;
        }

        string & getLastseen(){
            return lastseen;
        }

        void updateProfile(const string &name, const string &profilePicture){
            this->name = name;
            this->profilePicture = profilePicture;
        }

        void setLastSeen(string &lastseen){
            if(!lastseen.empty()){
                this->lastseen = lastseen;
            }

        }

        void setStatus(string &status){
            if(!status.empty()){    
                this->status = status;
            }
        }
    
};

// Contact 

class Contact {
    private: 
        string phone;
        string email;
        string address;
        string displayName;
    
    public:
        Contact(string &phone, string &email, string &address, string &displayName):
        phone(phone),
        email(email),
        address(address),
        displayName(displayName) {}

        string & getPhone(){
            return phone;
        }

        string & getEmail(){
            return email;
        }

        string & getAddress(){
            return address;
        }

        string & getDisplayName(){
            return displayName;
        }

        void updateContact(string &phone, string &email, string &address, string &displayName){
            if(!phone.empty()){
                this->phone = phone;
            }
            if(!email.empty()){
                this->email = email;
            }
            if(!address.empty()){
                this->address = address;
            }
            if(!displayName.empty()){
                this->displayName = displayName;
            }
        }
};

// Status

class Status {
    private: 
        int statusId;
        string content; 
        time_t expiryTime;
    
    public: 
        Status(int statusId, string &content, time_t expiryTime):
        statusId(statusId),
        content(content),
        expiryTime(expiryTime) {}

        int & getStatusId(){
            return statusId;
        }

        string & getContent(){
            return content;
        }

        time_t getExpiryTime(){
            return expiryTime;
        }

        bool isExpired(){
            return time(nullptr) >= expiryTime; // time(nullptr) : gives the current time as a time_t value.

            // Example:
            // Current time = 1760000000
            // Expiry time  = 1760003600

            // 1760000000 >= 1760003600 : false

        }
    
};

// Abstract Message 

class Message {
    protected: // so that child class can acess these 
        int messageId;
        shared_ptr<User> sender;

        // shared_ptr is a smart pointer from C++.
        // It automatically manages the lifetime of the object. When no shared_ptr is pointing to that User anymore, 
        // C++ automatically deletes the object.

        time_t timestamp;
        MessageStatusType status;

    public: 
        Message(int messageId, shared_ptr<User> sender):
        messageId(messageId),
        sender(sender),
        timestamp(time(nullptr)),
        status(MessageStatusType::SENT) {}

        virtual ~Message() = default;

        int getMessageId(){
            return messageId;
        }
        
        shared_ptr<User> getUser(){
            return sender;
        }

        MessageStatusType getStatus(){
            return status;
        }

        time_t getTimestamp(){
            return timestamp;
        }

        void markRead(){
            status = MessageStatusType::READ;
        }
        void markDelivered(){
            status = MessageStatusType::DELIVERED;
        }

        void markSent(){
            status = MessageStatusType::SENT;
        }

        virtual void edit(string &content) = 0;
        virtual void deleteMessage()=0;
        virtual void display() = 0;
        virtual string getSummary() = 0;

};

// Text Message

class TextMessage : public Message{
    private:
        string content;
    
    public: 
        TextMessage(int messageId, shared_ptr<User> sender, string &content):
        Message(messageId, move(sender)), // means -> "I don't need this local sender anymore; give it to the Message object."
        content(content){}

        string & getContent(){
            return content;
        }

        void edit(string &newContent){
            this->content = newContent;
        }

        void deleteMessage(){
            this->content = "[Message Deleted]";
        }

        void display(){
            cout << sender->getName() << ": " << content << '\n';
        }

        string getSummary(){
            return content;
        }
};
    


// Image Message

class MediaMessage : public Message{
    private:
        string mediaURL;
        MediaType mediaType;
    
    public: 
        MediaMessage(int messageId, shared_ptr<User> sender, string &mediaURL, MediaType mediaType):
        Message(messageId, move(sender)),
        mediaURL(mediaURL),
        mediaType(mediaType){}

        string & getMediaURL(){
            return mediaURL;
        }

        MediaType getMediaType(){
            return mediaType;
        }

        string getSummary(){
            return "[Media]: " + mediaURL;
        }

        string getSender(){
            return sender->getName();
        }

        void deleteMessage(){
            this->mediaURL = "[Media Deleted]";
        }

        void edit(string &newContent) override { 
            this->mediaURL = newContent;
        }

        void display(){
            cout << sender->getName() << ": " << mediaURL << '\n';
        }
};

// Abstract Chat

class Chat{
    protected:
        int chatId;
        vector <shared_ptr<Message>> messages;

    public:
        explicit Chat(int chatId):
        chatId(chatId){}

        virtual ~Chat() = default;


//      why needed explicit ? 
//      Chat chat = 101;  // ❌

//      C++ says:
//      No, you cannot automatically convert 101 into a Chat

//      But this is fine:
//      Chat chat(101);   // ✅

//      Because you're explicitly saying:
//      "I want to create a Chat with ID 101"

        int getChatId(){
            return chatId;
        }

        vector<shared_ptr<Message>> getMessages(){
            return messages;
        }

        void sendMessage(shared_ptr<Message> message){
            messages.push_back(move(message));
        }

        virtual void displayChat() = 0;

};

// Private chat

class PrivateChat : public Chat{
    protected:
        shared_ptr<User> user1;
        shared_ptr<User> user2;
    public:
        explicit  PrivateChat(int chatId, shared_ptr<User> user1, shared_ptr<User> user2):
        Chat(chatId),
        user1(user1),
        user2(user2){
            if(!this->user1 || !this->user2){
                throw invalid_argument("User cannot be null");
            }

            if(this->user1 == this->user2){
                throw invalid_argument("Private chat requires two different users");
            }
        }

        shared_ptr<User> getUser1(){
            return user1;
        }

        shared_ptr<User> getUser2(){
            return user2;
        }

        void displayChat(){
            cout<<"[Private Chat between] "<<user1->getName()<<" and "<<user2->getName()<<"\n";
            for(auto &message : messages){
                message->display();
            }
        }


};

// Group Chat

class GroupChat : public Chat{
    private:
        int groupId;
        string groupName;
        vector<shared_ptr<User>>members;
    
    public:
        explicit GroupChat(int chatId, string &groupName):
        Chat(chatId),
        groupName(groupName){}

        void displayChat(){
            cout<<"[Group Chat]"<<groupName<<"\n";
            for(auto &message : messages){
                message->display();
            }
        }

        vector<shared_ptr<User>> getMembers(){
            return members;
        }

        void addMember(shared_ptr<User> user){
            members.push_back(user);
        }

        bool removeMember(int &userId){
            int oldSize = members.size();
            members.erase(
                remove_if(members.begin(),
                    members.end(),
                    [userId](shared_ptr<User> user){
                        return user->getUserId() == userId;
                    }
                ),
                members.end()
            );

            return oldSize != members.size();
        }
};

// Message Service 

class MessageService{
        
    public:
        void sendMessage(shared_ptr<Chat> chat, shared_ptr<Message> message){
            if(!message){
                throw invalid_argument( "Message cannot be null");
            }
            message->markSent();
            chat->sendMessage(move(message));
        }

        void markDelivered(shared_ptr<Message> message){
            if(!message){
                throw invalid_argument( "Message cannot be null");
            }
            message->markDelivered();
        }

        void markRead(shared_ptr<Message> message){
            if(!message){
                throw invalid_argument( "Message cannot be null");
            }
            message->markRead();
        }

        void deleteMessage(shared_ptr<Message> message){
            if(!message){
                throw invalid_argument( "Message cannot be null");
            }
            message->deleteMessage();
        }

        void editMessage(shared_ptr<Message> message, string &content){
            if(!message){
                throw invalid_argument( "Message cannot be null");
            }
            message->edit(content);
        }


};

// Notification

class Notification{
    private:
        shared_ptr<User> receiver;
        shared_ptr<Message> message;
        NotificationType type;
    
    public:
        explicit Notification(shared_ptr<User> receiver, shared_ptr<Message> message, NotificationType type):
        receiver(receiver),
        message(message),
        type(type){
            if(!receiver || !message){
                throw invalid_argument("Receiver or message cannot be null");
            }
        }

        void sendNotification(){
            cout<<"[Notification]: "<<receiver->getName()<<" received message: "<<message->getSummary()<<'\n';
            
        }

};

// Notification Service

class NotificationService {
    public:

        void notify(shared_ptr<User> &receiver, shared_ptr<Message> message, NotificationType type){
            Notification notification(receiver, message, type);
            notification.sendNotification();
        }

        void notifyGroup(GroupChat & groupChat, shared_ptr<Message> &message, NotificationType type){
            cout<<"[Group Notifications]: "<<'\n';
            for(auto& receiver : groupChat.getMembers()){
                if(receiver->getUserId()== message->getUser()->getUserId()) continue;
                notify(receiver, message, type);
            }
        }
};

// Backup

class Backup {
    private:
        vector<shared_ptr<Message>> messages;
        time_t timestamp;
    public:
        explicit Backup(vector<shared_ptr<Message>> messages):
        messages(messages),
        timestamp(time(nullptr)){
            if(messages.empty()){
                throw invalid_argument("Messages cannot be empty");
            }
        }

        void backup(){
            cout<<"[Backup]: "<<messages.size()<<" messages backed up"<<endl;
        }

        void restore(){
            cout<<"[Restore]: "<<messages.size()<<" messages restored"<<endl;
        }

};


// User Tests 

void testUser(){
    string name = "Shikshak";

    shared_ptr<User> user = make_shared<User>(1,name); // id,name

    // assert() basically means:
    //"I expect this condition to be TRUE. If it is false, the test fails."
    
    assert(user->getUserId()==1);
    assert(user->getName()=="Shikshak");
    assert(user->getProfilePicture()=="");
    assert(user->getLastseen()=="");
    assert(user->getStatus()=="");

    string status = "Available";
    user->setStatus(status);
    assert(user->getStatus()=="Available");

    string lastseen = "5 mins ago";
    user->setLastSeen(lastseen);
    assert(user->getLastseen()=="5 mins ago");



    user->updateProfile("Shikshak Kumar", "profile.jpg");
    assert(user->getName()=="Shikshak Kumar");
    assert(user->getProfilePicture()=="profile.jpg");

    cout<<"User tests passed!"<<endl;
}

void testContact(){
    string phone = "+91234567890";
    string email = "test@gmail.com";
    string address = "123, Test Street";
    string displayName = "Test User";

    Contact contact(phone, email, address, displayName); // implemented this way so that it can be destroyed after this function ends, and not before.

    assert(contact.getPhone()=="+91234567890");
    assert(contact.getEmail()=="test@gmail.com");
    assert(contact.getAddress()=="123, Test Street");
    assert(contact.getDisplayName()=="Test User");

    string newPhone = "+9876543210";
    string newEmail = "shikshak@gmail.com";
    string newAddress = "Bangalore, India";
    string newName = "Shikshak";
    contact.updateContact(newPhone, newEmail, newAddress, newName);

    assert(contact.getPhone()=="+9876543210");
    assert((contact.getEmail() == "shikshak@gmail.com") && "Email clear failed!");
    assert(contact.getAddress()=="Bangalore, India");
    assert((contact.getDisplayName()=="Shikshak") && "Display name should not have changed");

    cout<<"Contact tests passed!"<<endl;
}

void testStatus(){
    int statusId = 1;
    string content = "Busy";
    time_t expiry = time(nullptr)+60;
    

    Status status(statusId, content, expiry);

    time_t futureTime = time(nullptr)+120;

    assert(status.getStatusId()==1);
    assert(status.getContent()=="Busy");
    assert(status.getExpiryTime()==expiry);
    assert(expiry < futureTime && "Expiry time should be in the future");
    assert(!status.isExpired());

    cout<<"Status tests passed!"<<endl;

}

void testTextMessage(){
    int messageId = 101;

    string userName = "Shikshak";
    int userId = 1;
    shared_ptr<User> sender = make_shared<User>(userId,userName);

    string content = "hello how are you";

    auto textMessage = make_shared<TextMessage>(messageId, sender, content);

    assert(textMessage->getContent()=="hello how are you");
    assert(textMessage->getSummary()=="hello how are you");

    textMessage->markDelivered();
    assert(textMessage->getStatus()==MessageStatusType::DELIVERED);

    string newMessage = "I am fine";

    textMessage->edit(newMessage);
    assert(textMessage->getContent()=="I am fine");

    textMessage->deleteMessage();
    assert(textMessage->getContent()=="[Message Deleted]");

    cout<<"Text message tests passed!"<<endl;
}

void testMediaMessage(){
    int messageId = 101;

    string userName = "Shikshak";
    int userId = 1;
    shared_ptr<User> sender = make_shared<User>(userId,userName);

    string mediaURL = "https://example.com/image.jpg";

    auto mediaMessage = make_shared<MediaMessage>(messageId, sender, mediaURL, MediaType::IMAGE);

    assert(mediaMessage->getMediaURL()=="https://example.com/image.jpg");
    assert(mediaMessage->getSummary()=="[Media]: https://example.com/image.jpg");
    assert(mediaMessage->getMediaType()==MediaType::IMAGE);

    mediaMessage->markSent();
    mediaMessage->markDelivered();

    assert(mediaMessage->getStatus()==MessageStatusType::DELIVERED);

    string newMediaURL = "https://example.com/new_image.jpg";

    mediaMessage->edit(newMediaURL);

    // mediaMessage->display();

    assert(mediaMessage->getMediaURL()=="https://example.com/new_image.jpg");

    cout<<"Media message tests passed!"<<endl;

}

void testPrivateChat(){
    string user1Name = "Shikshak";
    string user2Name = "Prince";

    shared_ptr<User> user1 = make_shared<User>(1,user1Name);
    shared_ptr<User> user2 = make_shared<User>(2,user2Name);

    int chatId = 1;

    PrivateChat privateChat(chatId, user1, user2);

    
    assert(privateChat.getChatId()==1);

    string messageFromUser1 = "Hello Prince!";
    shared_ptr<Message> message1 = make_shared<TextMessage>(1, user1, messageFromUser1);
    privateChat.sendMessage(message1);


    string messageFromUser2 = "Hello Shikshak!";
    shared_ptr<Message> message2 = make_shared<TextMessage>(2, user2, messageFromUser2);
    privateChat.sendMessage(message2);

    privateChat.displayChat();

}

void testGroupChat(){
    string user1Name = "Shikshak";
    string user2Name = "Prince";
    string user3Name = "Priyam";
    string user4Name = "Ayush";

    shared_ptr<User> user1 = make_shared<User>(1,user1Name);
    shared_ptr<User> user2 = make_shared<User>(2,user2Name);
    shared_ptr<User> user3 = make_shared<User>(3,user3Name);
    shared_ptr<User> user4 = make_shared<User>(4,user4Name);

    string groupName = "Pg";

    GroupChat groupChat(1, groupName);

    groupChat.addMember(user1);
    groupChat.addMember(user2);
    groupChat.addMember(user3);
    groupChat.addMember(user4);

    string messageFromUser1 = "Bhai lassie le aao";
    shared_ptr<Message> message1 = make_shared<TextMessage>(1, user1, messageFromUser1);
    groupChat.sendMessage(message1);


    string messageFromUser2 = "Chal bhai sath me";
    shared_ptr<Message> message2 = make_shared<TextMessage>(2, user2, messageFromUser2);
    groupChat.sendMessage(message2);

    string messageFromUser3 = "Mere liye bhi le aaio";
    shared_ptr<Message> message3 = make_shared<TextMessage>(3, user3, messageFromUser3);
    groupChat.sendMessage(message3);

    string messageFromUser4 = "bhej paise";
    shared_ptr<Message> message4 = make_shared<TextMessage>(4, user4, messageFromUser4);
    groupChat.sendMessage(message4);

    assert(groupChat.removeMember(user4->getUserId()));

    groupChat.displayChat();

    cout<<"Group chat tests passed!"<<endl;
}

void testNotification(){
    string receiverName = "Shikshak";
    shared_ptr<User> receiver = make_shared<User>(1,receiverName);
    string content = "Hello Shikshak";
    shared_ptr<Message> message = make_shared<TextMessage>(1, receiver, content);
    NotificationType type = NotificationType::MESSAGE;

    Notification notification(receiver, message, type);

    // notification.sendNotification();

    cout<<"Notification tests passed!"<<endl;
}

void testNotificationService(){
    string receiverName = "Shikshak";
    shared_ptr<User> receiver = make_shared<User>(1,receiverName);
    string content = "Hello Shikshak";
    shared_ptr<Message> message = make_shared<TextMessage>(1, receiver, content);
    NotificationType type = NotificationType::MESSAGE;

    NotificationService service;

    service.notify(receiver, message, type);


    string user1Name = "Shikshak";
    string user2Name = "Prince";
    string user3Name = "Priyam";
    string user4Name = "Ayush";

    shared_ptr<User> user1 = make_shared<User>(1,user1Name);
    shared_ptr<User> user2 = make_shared<User>(2,user2Name);
    shared_ptr<User> user3 = make_shared<User>(3,user3Name);
    shared_ptr<User> user4 = make_shared<User>(4,user4Name);

    string groupName = "Pg";

    GroupChat groupChat(1, groupName);

    groupChat.addMember(user1);
    groupChat.addMember(user2);
    groupChat.addMember(user3);
    groupChat.addMember(user4);

    string messageFromUser1 = "Bhai lassie le aao";
    shared_ptr<Message> message1 = make_shared<TextMessage>(1, user1, messageFromUser1);
    groupChat.sendMessage(message1);

    service.notifyGroup(groupChat,message1,type);


    string messageFromUser2 = "Chal bhai sath me";
    shared_ptr<Message> message2 = make_shared<TextMessage>(2, user2, messageFromUser2);
    groupChat.sendMessage(message2);

    string messageFromUser3 = "Mere liye bhi le aaio";
    shared_ptr<Message> message3 = make_shared<TextMessage>(3, user3, messageFromUser3);
    groupChat.sendMessage(message3);

    string messageFromUser4 = "bhej paise";
    shared_ptr<Message> message4 = make_shared<TextMessage>(4, user4, messageFromUser4);
    groupChat.sendMessage(message4);

    
    cout<<"Notification Service test passed!"<<endl;
    
}

void testMessageService(){
    string name1 = "Shikshak";
    string name2 = "Prince";

    shared_ptr<User> user1 = make_shared<User>(1,name1);
    shared_ptr<User> user2 = make_shared<User>(2,name2);

    auto chat = make_shared<PrivateChat>(1, user1, user2);

    string content = "hello prince";
    shared_ptr<TextMessage> message = make_shared<TextMessage>(1,user1,content);

    MessageService service;

    service.sendMessage(chat,message);
    assert(chat->getMessages().size()==1);

    assert(message->getStatus()==MessageStatusType::SENT);

    service.markDelivered(message);
    assert(message->getStatus() == MessageStatusType::DELIVERED);

    service.markRead(message);
    assert(message->getStatus() == MessageStatusType::READ);

    string newContent = "hello prince, how are you?";

    service.editMessage(message,newContent);
    assert(message->getContent() == "hello prince, how are you?");

    service.deleteMessage(message);
    assert(message->getContent() == "[Message Deleted]");

    cout<<"Message Service test passed!"<<endl;

}

void testBackup(){
    string name = "Shikshak";
    auto user = make_shared<User>(1,name);
    string content = "hello there";
    auto message = make_shared<TextMessage>(101,user,content);

    vector<shared_ptr<Message>> messages = {message};

    Backup backup(messages);

    backup.backup();
    backup.restore();

    cout<<"Backup test passed!"<<endl;
}


void runAllTests(){
    testUser();
    testContact();
    testStatus();
    testTextMessage();
    testMediaMessage();
    testPrivateChat();
    testGroupChat();
    testNotification();
    testNotificationService();
    testMessageService();
    testBackup();
}

int main(){
    runAllTests();
    return 0;
}