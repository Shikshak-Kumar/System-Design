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

        void updateProfile(string &name, string &profilePicture){
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
        
        shared_ptr<User> getuser(){
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
        explicit GroupChat(int chatId, string &groupName, vector<shared_ptr<User>> &members):
        Chat(chatId),
        groupName(groupName),
        members(members){}

        void displayChat(){
            cout<<"[Group Chat]"<<groupName<<"\n";
            for(auto &message : messages){
                message->display();
            }
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

class NotificationService{
    public:

        void notify(shared_ptr<User> &receiver, shared_ptr<Message> message, NotificationType type){
            Notification notification(receiver, message, type);
            notification.sendNotification();
        }

        void notify(vector<shared_ptr<User>> &receivers, shared_ptr<Message> &message, NotificationType type){
            for(auto& receiver : receivers){
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


// ============================================================
// TESTS (Compatible with practice.cpp signatures)
// ============================================================

// ------------------------------------------------------------
// 1. User Tests
// ------------------------------------------------------------
void testUser() {
    string name = "Shikshak";
    auto user = make_shared<User>(1, name);

    assert(user->getUserId() == 1);
    assert(user->getName() == "Shikshak");
    assert(user->getStatus().empty());
    assert(user->getProfilePicture().empty());
    assert(user->getLastseen().empty());

    string status = "Available";
    user->setStatus(status);
    assert(user->getStatus() == "Available");

    string lastSeen = "10:30 PM";
    user->setLastSeen(lastSeen);
    assert(user->getLastseen() == "10:30 PM");

    string newName = "Shikshak Kumar";
    string newPic = "profile.jpg";
    user->updateProfile(newName, newPic);
    assert(user->getName() == "Shikshak Kumar");
    assert(user->getProfilePicture() == "profile.jpg");

    // Empty status / lastseen edge cases (should not overwrite)
    string emptyStr = "";
    user->setStatus(emptyStr);
    assert(user->getStatus() == "Available");
    user->setLastSeen(emptyStr);
    assert(user->getLastseen() == "10:30 PM");

    cout << "testUser PASSED\n";
}

// ------------------------------------------------------------
// 2. Contact Tests
// ------------------------------------------------------------
void testContact() {
    string phone = "+1234567890";
    string email = "shikshak@example.com";
    string address = "New Delhi, India";
    string displayName = "Shikshak Kumar";

    Contact contact(phone, email, address, displayName);

    assert(contact.getPhone() == "+1234567890");
    assert(contact.getEmail() == "shikshak@example.com");
    assert(contact.getAddress() == "New Delhi, India");
    assert(contact.getDisplayName() == "Shikshak Kumar");

    string newPhone = "+9876543210";
    string emptyEmail = "";
    string newAddress = "Bangalore, India";
    string emptyName = "";
    contact.updateContact(newPhone, emptyEmail, newAddress, emptyName);

    // Verified partial updates
    assert(contact.getPhone() == "+9876543210");
    assert(contact.getEmail() == "shikshak@example.com"); // preserved
    assert(contact.getAddress() == "Bangalore, India");
    assert(contact.getDisplayName() == "Shikshak Kumar");  // preserved

    cout << "testContact PASSED\n";
}

// ------------------------------------------------------------
// 3. Status Tests
// ------------------------------------------------------------
void testStatus() {
    string content1 = "Working";
    Status status(1, content1, time(nullptr) + 100);

    assert(status.getStatusId() == 1);
    assert(status.getContent() == "Working");
    assert(!status.isExpired());

    string content2 = "Old Status";
    Status expiredStatus(2, content2, time(nullptr) - 100);

    assert(expiredStatus.getStatusId() == 2);
    assert(expiredStatus.isExpired());

    cout << "testStatus PASSED\n";
}

// ------------------------------------------------------------
// 4. Text Message Tests
// ------------------------------------------------------------
void testTextMessage() {
    string name = "Shikshak";
    auto user = make_shared<User>(1, name);

    string content = "Hello";
    auto textMessage = make_shared<TextMessage>(101, user, content);

    assert(textMessage->getMessageId() == 101);
    assert(textMessage->getuser() == user);
    assert(textMessage->getContent() == "Hello");
    assert(textMessage->getSummary() == "Hello");
    assert(textMessage->getStatus() == MessageStatusType::SENT);

    // Status transitions
    textMessage->markDelivered();
    assert(textMessage->getStatus() == MessageStatusType::DELIVERED);

    textMessage->markRead();
    assert(textMessage->getStatus() == MessageStatusType::READ);

    // Edit message
    string updatedContent = "Hello World";
    textMessage->edit(updatedContent);
    assert(textMessage->getContent() == "Hello World");

    // Delete message
    textMessage->deleteMessage();
    assert(textMessage->getContent() == "[Message Deleted]");

    cout << "testTextMessage PASSED\n";
}

// ------------------------------------------------------------
// 5. Media Message Tests
// ------------------------------------------------------------
void testMediaMessage() {
    string name = "Shikshak";
    auto user = make_shared<User>(1, name);

    string url = "photo.png";
    auto mediaMessage = make_shared<MediaMessage>(102, user, url, MediaType::IMAGE);

    assert(mediaMessage->getMessageId() == 102);
    assert(mediaMessage->getuser() == user);
    assert(mediaMessage->getMediaURL() == "photo.png");
    assert(mediaMessage->getMediaType() == MediaType::IMAGE);
    assert(mediaMessage->getSummary() == "[Media]: photo.png");

    string newUrl = "video.mp4";
    mediaMessage->edit(newUrl);
    assert(mediaMessage->getMediaURL() == "video.mp4");

    mediaMessage->deleteMessage();
    assert(mediaMessage->getMediaURL() == "[Media Deleted]");

    cout << "testMediaMessage PASSED\n";
}

// ------------------------------------------------------------
// 6. Private Chat Tests & Edge Cases
// ------------------------------------------------------------
void testPrivateChat() {
    string name1 = "User1", name2 = "User2";
    auto user1 = make_shared<User>(1, name1);
    auto user2 = make_shared<User>(2, name2);

    PrivateChat chat(201, user1, user2);
    assert(chat.getChatId() == 201);
    assert(chat.getUser1() == user1);
    assert(chat.getUser2() == user2);
    assert(chat.getMessages().empty());

    // Validation: same user throws invalid_argument
    bool caughtSameUser = false;
    try {
        PrivateChat invalidChat(202, user1, user1);
    } catch (const invalid_argument&) {
        caughtSameUser = true;
    }
    assert(caughtSameUser);

    // Validation: null user throws invalid_argument
    bool caughtNullUser = false;
    try {
        PrivateChat invalidChat2(203, user1, nullptr);
    } catch (const invalid_argument&) {
        caughtNullUser = true;
    }
    assert(caughtNullUser);

    cout << "testPrivateChat PASSED\n";
}

// ------------------------------------------------------------
// 7. Group Chat Tests
// ------------------------------------------------------------
void testGroupChat() {
    string name1 = "User1", name2 = "User2", name3 = "User3";
    auto user1 = make_shared<User>(1, name1);
    auto user2 = make_shared<User>(2, name2);
    auto user3 = make_shared<User>(3, name3);

    string groupName = "Design Discussion";
    vector<shared_ptr<User>> members = {user1, user2};

    GroupChat group(301, groupName, members);
    assert(group.getChatId() == 301);

    // Add member
    group.addMember(user3);

    // Remove member (takes int&)
    int idToRemove = 2;
    assert(group.removeMember(idToRemove) == true);

    // Remove non-existent member
    int nonExistentId = 999;
    assert(group.removeMember(nonExistentId) == false);

    cout << "testGroupChat PASSED\n";
}

// ------------------------------------------------------------
// 8. Message Service Tests
// ------------------------------------------------------------
void testMessageService() {
    string name1 = "User1", name2 = "User2";
    auto user1 = make_shared<User>(1, name1);
    auto user2 = make_shared<User>(2, name2);

    auto chat = make_shared<PrivateChat>(401, user1, user2);
    MessageService service;

    string content = "Hi!";
    auto message = make_shared<TextMessage>(501, user1, content);

    // Send message via service
    service.sendMessage(chat, message);
    assert(chat->getMessages().size() == 1);
    assert(message->getStatus() == MessageStatusType::SENT);

    // Lifecycle transitions through service
    service.markDelivered(message);
    assert(message->getStatus() == MessageStatusType::DELIVERED);

    service.markRead(message);
    assert(message->getStatus() == MessageStatusType::READ);

    string edited = "Hello!";
    service.editMessage(message, edited);
    assert(message->getContent() == "Hello!");

    service.deleteMessage(message);
    assert(message->getContent() == "[Message Deleted]");

    // Null validations
    bool caughtNullMessage = false;
    try {
        service.sendMessage(chat, nullptr);
    } catch (const invalid_argument&) {
        caughtNullMessage = true;
    }
    assert(caughtNullMessage);

    cout << "testMessageService PASSED\n";
}

// ------------------------------------------------------------
// 9. Backup Tests
// ------------------------------------------------------------
void testBackup() {
    string name = "User1";
    auto user = make_shared<User>(1, name);
    string content = "Msg";
    auto message = make_shared<TextMessage>(601, user, content);

    vector<shared_ptr<Message>> messages = {message};
    Backup backup(messages);

    // Test output operations
    backup.backup();
    backup.restore();

    // Validation: empty messages vector must throw invalid_argument
    bool caughtEmptyBackup = false;
    try {
        vector<shared_ptr<Message>> emptyList;
        Backup invalidBackup(emptyList);
    } catch (const invalid_argument&) {
        caughtEmptyBackup = true;
    }
    assert(caughtEmptyBackup);

    cout << "testBackup PASSED\n";
}

// ------------------------------------------------------------
// 10. Runner Harness
// ------------------------------------------------------------
void runAllTests() {
    cout << "\n========== RUNNING TESTS ==========\n\n";

    testUser();
    testContact();
    testStatus();
    testTextMessage();
    testMediaMessage();
    testPrivateChat();
    testGroupChat();
    testMessageService();
    testBackup();

    cout << "\n========== ALL TESTS PASSED ==========\n";
}




int main() {
	// your code goes here
    runAllTests();
    return 0;;
}
