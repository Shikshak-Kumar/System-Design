#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <cassert>
#include <ctime>

using namespace std;

// ============================================================
// ENUMS
// ============================================================

enum class MessageStatus {
    SENT,
    DELIVERED,
    READ
};

enum class MediaType {
    IMAGE,
    VIDEO,
    AUDIO,
    DOCUMENT
};

enum class NotificationType {
    MESSAGE,
    GROUP
};


// ============================================================
// USER
// ============================================================

class User {
private:
    int userId;
    string name;
    string profilePicture;
    string status;
    string lastSeenTime;

public:
    User(int userId, const string& name)
        : userId(userId),
          name(name),
          profilePicture(""),
          status("Hey there!"),
          lastSeenTime("") {}

    // Getters
    int getUserId() const {
        return userId;
    }

    const string& getName() const {
        return name;
    }

    const string& getProfilePicture() const {
        return profilePicture;
    }

    const string& getStatus() const {
        return status;
    }

    const string& getLastSeenTime() const {
        return lastSeenTime;
    }

    // Operations
    void updateProfile(
        const string& newName,
        const string& newProfilePicture) {

        name = newName;
        profilePicture = newProfilePicture;
    }

    void setStatus(const string& newStatus) {
        status = newStatus;
    }

    void setLastSeen(const string& time) {
        lastSeenTime = time;
    }
};


// ============================================================
// CONTACT
// ============================================================

class Contact {
private:
    string phoneNumber;
    string email;
    string address;
    string displayName;

public:
    Contact(
        const string& phoneNumber,
        const string& email,
        const string& address,
        const string& displayName)
        : phoneNumber(phoneNumber),
          email(email),
          address(address),
          displayName(displayName) {}

    const string& getPhoneNumber() const {
        return phoneNumber;
    }

    const string& getEmail() const {
        return email;
    }

    const string& getAddress() const {
        return address;
    }

    const string& getDisplayName() const {
        return displayName;
    }
};


// ============================================================
// STATUS
// ============================================================

class Status {
private:
    int statusId;
    string content;
    time_t expiryTime;

public:
    Status(
        int statusId,
        const string& content,
        time_t expiryTime)
        : statusId(statusId),
          content(content),
          expiryTime(expiryTime) {}

    int getStatusId() const {
        return statusId;
    }

    const string& getContent() const {
        return content;
    }

    bool isExpired() const {
        return time(nullptr) >= expiryTime;
    }
};


// ============================================================
// ABSTRACT MESSAGE
// ============================================================

class Message {
protected:
    int messageId;
    shared_ptr<User> sender;
    time_t timestamp;
    MessageStatus status;

public:
    Message(
        int messageId,
        shared_ptr<User> sender)
        : messageId(messageId),
          sender(std::move(sender)),
          timestamp(time(nullptr)),
          status(MessageStatus::SENT) {}

    virtual ~Message() = default;

    int getMessageId() const {
        return messageId;
    }

    shared_ptr<User> getSender() const {
        return sender;
    }

    MessageStatus getStatus() const {
        return status;
    }

    void markDelivered() {
        status = MessageStatus::DELIVERED;
    }

    void markRead() {
        status = MessageStatus::READ;
    }

    // Polymorphic operations
    virtual void edit(const string& newContent) = 0;

    virtual void deleteMessage() = 0;

    virtual void display() const = 0;

    virtual string getSummary() const = 0;
};


// ============================================================
// TEXT MESSAGE
// ============================================================

class TextMessage : public Message {
private:
    string content;

public:
    TextMessage(
        int messageId,
        shared_ptr<User> sender,
        const string& content)
        : Message(messageId, std::move(sender)),
          content(content) {}

    const string& getContent() const {
        return content;
    }

    string getSummary() const override {
        return content;
    }

    void edit(const string& newContent) override {
        content = newContent;
    }

    void deleteMessage() override {
        content = "[Message deleted]";
    }

    void display() const override {
        cout << sender->getName()
             << ": " << content << '\n';
    }
};


// ============================================================
// MEDIA MESSAGE
// ============================================================

class MediaMessage : public Message {
private:
    string mediaUrl;
    MediaType mediaType;

public:
    MediaMessage(
        int messageId,
        shared_ptr<User> sender,
        const string& mediaUrl,
        MediaType mediaType)
        : Message(messageId, std::move(sender)),
          mediaUrl(mediaUrl),
          mediaType(mediaType) {}

    const string& getMediaUrl() const {
        return mediaUrl;
    }

    MediaType getMediaType() const {
        return mediaType;
    }

    string getSummary() const override {
        return "[Media] " + mediaUrl;
    }

    // For media, edit can mean replacing the media URL.
    void edit(const string& newUrl) override {
        mediaUrl = newUrl;
    }

    void deleteMessage() override {
        mediaUrl = "[Media deleted]";
    }

    void display() const override {
        cout << sender->getName()
             << ": " << getSummary() << '\n';
    }
};


// ============================================================
// ABSTRACT CHAT
// ============================================================

class Chat {
protected:
    int chatId;

    vector<shared_ptr<User>> participants;

    // Composition:
    // Chat owns its messages.
    vector<shared_ptr<Message>> messages;

public:
    explicit Chat(int chatId)
        : chatId(chatId) {}

    virtual ~Chat() = default;

    int getChatId() const {
        return chatId;
    }

    const vector<shared_ptr<User>>& getParticipants() const {
        return participants;
    }

    const vector<shared_ptr<Message>>& getMessages() const {
        return messages;
    }

    virtual bool addUser(shared_ptr<User> user) = 0;

    virtual bool removeUser(int userId) = 0;

    void sendMessage(shared_ptr<Message> message) {
        if (!message) {
            throw invalid_argument("Message cannot be null");
        }

        messages.push_back(std::move(message));
    }

    void displayMessagesForUser(const shared_ptr<User>& user) const {
        if (!user) return;
        for (const auto& message : messages) {
            if (message->getSender()->getUserId() != user->getUserId()) {
                cout << "  [" << user->getName() << "'s Screen] From "
                     << message->getSender()->getName() << ": "
                     << message->getSummary() << '\n';
            }
        }
    }

    // Pure virtual => Chat is abstract
    virtual void displayChat() const = 0;
};


// ============================================================
// PRIVATE CHAT
// ============================================================

class PrivateChat : public Chat {

public:
    PrivateChat(
        int chatId,
        shared_ptr<User> user1,
        shared_ptr<User> user2)
        : Chat(chatId) {

        if (!user1 || !user2) {
            throw invalid_argument(
                "Private chat users cannot be null");
        }

        if (user1->getUserId() == user2->getUserId()) {
            throw invalid_argument(
                "Private chat requires two different users");
        }

        participants.push_back(std::move(user1));
        participants.push_back(std::move(user2));
    }

    bool addUser(shared_ptr<User> user) override {

        if (!user) {
            return false;
        }

        // Private chat can only have 2 users.
        if (participants.size() >= 2) {
            return false;
        }

        participants.push_back(std::move(user));
        return true;
    }

    bool removeUser(int userId) override {

        auto oldSize = participants.size();

        participants.erase(
            remove_if(
                participants.begin(),
                participants.end(),
                [userId](const shared_ptr<User>& user) {
                    return user->getUserId() == userId;
                }),
            participants.end());

        return participants.size() != oldSize;
    }

    void displayChat() const override {

        cout << "Private Chat #" << chatId << '\n';

        for (const auto& user : participants) {
            cout << "  " << user->getName() << '\n';
        }
    }
};


// ============================================================
// GROUP CHAT
// ============================================================

class GroupChat : public Chat {
private:
    string groupName;
    vector<shared_ptr<User>> admins;

public:
    GroupChat(
        int chatId,
        const string& groupName)
        : Chat(chatId),
          groupName(groupName) {}

    const string& getGroupName() const {
        return groupName;
    }

    bool addUser(shared_ptr<User> user) override {

        if (!user) {
            return false;
        }

        // Prevent duplicate participants.
        for (const auto& participant : participants) {
            if (participant->getUserId() ==
                user->getUserId()) {
                return false;
            }
        }

        participants.push_back(std::move(user));
        return true;
    }

    bool removeUser(int userId) override {

        auto oldSize = participants.size();

        participants.erase(
            remove_if(
                participants.begin(),
                participants.end(),
                [userId](const shared_ptr<User>& user) {
                    return user->getUserId() == userId;
                }),
            participants.end());

        // Also remove from admins.
        admins.erase(
            remove_if(
                admins.begin(),
                admins.end(),
                [userId](const shared_ptr<User>& user) {
                    return user->getUserId() == userId;
                }),
            admins.end());

        return participants.size() != oldSize;
    }

    bool addAdmin(shared_ptr<User> user) {

        if (!user) {
            return false;
        }

        // User must already be a participant.
        bool isParticipant = false;

        for (const auto& participant : participants) {
            if (participant->getUserId() ==
                user->getUserId()) {

                isParticipant = true;
                break;
            }
        }

        if (!isParticipant) {
            return false;
        }

        // Prevent duplicate admin.
        for (const auto& admin : admins) {
            if (admin->getUserId() ==
                user->getUserId()) {
                return false;
            }
        }

        admins.push_back(std::move(user));
        return true;
    }

    bool removeAdmin(int userId) {

        auto oldSize = admins.size();

        admins.erase(
            remove_if(
                admins.begin(),
                admins.end(),
                [userId](const shared_ptr<User>& user) {
                    return user->getUserId() == userId;
                }),
            admins.end());

        return admins.size() != oldSize;
    }

    const vector<shared_ptr<User>>& getAdmins() const {
        return admins;
    }

    void displayChat() const override {

        cout << "Group Chat #" << chatId
             << " : " << groupName << '\n';

        cout << "Participants:\n";

        for (const auto& user : participants) {
            cout << "  " << user->getName() << '\n';
        }

        cout << "Admins:\n";

        for (const auto& admin : admins) {
            cout << "  " << admin->getName() << '\n';
        }
    }
};


// ============================================================
// MESSAGE SERVICE
// ============================================================

class MessageService {
public:

    shared_ptr<TextMessage> createTextMessage(
        int messageId,
        shared_ptr<User> sender,
        const string& content) {

        return make_shared<TextMessage>(
            messageId,
            std::move(sender),
            content);
    }

    shared_ptr<MediaMessage> createMediaMessage(
        int messageId,
        shared_ptr<User> sender,
        const string& url,
        MediaType type) {

        return make_shared<MediaMessage>(
            messageId,
            std::move(sender),
            url,
            type);
    }

    void sendMessage(
        const shared_ptr<Chat>& chat,
        const shared_ptr<Message>& message) {

        if (!chat || !message) {
            throw invalid_argument(
                "Chat and message cannot be null");
        }

        chat->sendMessage(message);
    }

    void editMessage(
        const shared_ptr<Message>& message,
        const string& newContent) {

        if (!message) {
            throw invalid_argument(
                "Message cannot be null");
        }

        message->edit(newContent);
    }

    void deleteMessage(
        const shared_ptr<Message>& message) {

        if (!message) {
            throw invalid_argument(
                "Message cannot be null");
        }

        message->deleteMessage();
    }
};


// ============================================================
// NOTIFICATION
// ============================================================

class Notification {
private:
    NotificationType type;
    string message;
    shared_ptr<User> recipient;
    shared_ptr<User> sender;
    time_t timestamp;

public:
    Notification(
        NotificationType type,
        const string& message,
        shared_ptr<User> recipient = nullptr,
        shared_ptr<User> sender = nullptr)
        : type(type),
          message(message),
          recipient(std::move(recipient)),
          sender(std::move(sender)),
          timestamp(time(nullptr)) {}

    NotificationType getType() const {
        return type;
    }

    const string& getMessage() const {
        return message;
    }

    shared_ptr<User> getRecipient() const {
        return recipient;
    }

    shared_ptr<User> getSender() const {
        return sender;
    }

    time_t getTimestamp() const {
        return timestamp;
    }
};


// ============================================================
// NOTIFICATION SERVICE
// ============================================================

class NotificationService {
private:
    vector<Notification> notifications;

public:

    void addNotification(
        const Notification& notification) {

        notifications.push_back(notification);
    }

    void sendNotification() const {

        for (const auto& notification : notifications) {

            cout << "Notification: ";
            if (notification.getRecipient()) {
                cout << notification.getRecipient()->getName() << ", ";
            }
            cout << notification.getMessage();

            if (notification.getSender()) {
                cout << " (Sent by: "
                     << notification.getSender()->getName()
                     << ")";
            }

            cout << '\n';
        }
    }

    size_t getNotificationCount() const {
        return notifications.size();
    }
};


// ============================================================
// BACKUP
// ============================================================

class Backup {
private:
    int backupId;
    time_t lastBackupTime;

public:
    explicit Backup(int backupId)
        : backupId(backupId),
          lastBackupTime(0) {}

    int getBackupId() const {
        return backupId;
    }

    time_t getLastBackupTime() const {
        return lastBackupTime;
    }

    void createBackup() {
        lastBackupTime = time(nullptr);
    }

    bool restoreBackup() const {
        return lastBackupTime != 0;
    }
};


// ============================================================
// TESTS
// ============================================================

// ------------------------------------------------------------
// User Tests
// ------------------------------------------------------------

void testUser() {

    auto user = make_shared<User>(
        1,
        "Shikshak");

    assert(user->getUserId() == 1);
    assert(user->getName() == "Shikshak");
    assert(user->getStatus() == "Hey there!");
    assert(user->getProfilePicture().empty());
    assert(user->getLastSeenTime().empty());

    user->setStatus("Available");
    assert(user->getStatus() == "Available");

    user->setLastSeen("10:30 PM");
    assert(user->getLastSeenTime() == "10:30 PM");

    user->updateProfile(
        "Shikshak Kumar",
        "profile.jpg");

    assert(user->getName() == "Shikshak Kumar");
    assert(user->getProfilePicture() == "profile.jpg");

    cout << "testUser PASSED\n";
}


// ------------------------------------------------------------
// Contact Tests
// ------------------------------------------------------------

void testContact() {

    Contact contact(
        "+1234567890",
        "shikshak@example.com",
        "New Delhi, India",
        "Shikshak Kumar");

    assert(contact.getPhoneNumber() == "+1234567890");
    assert(contact.getEmail() == "shikshak@example.com");
    assert(contact.getAddress() == "New Delhi, India");
    assert(contact.getDisplayName() == "Shikshak Kumar");

    cout << "testContact PASSED\n";
}


// ------------------------------------------------------------
// Status Tests
// ------------------------------------------------------------

void testStatus() {

    Status status(
        1,
        "Working",
        time(nullptr) + 100);

    assert(status.getStatusId() == 1);
    assert(status.getContent() == "Working");
    assert(!status.isExpired());

    Status expiredStatus(
        2,
        "Old status",
        time(nullptr) - 100);

    assert(expiredStatus.getStatusId() == 2);
    assert(expiredStatus.isExpired());

    cout << "testStatus PASSED\n";
}


// ------------------------------------------------------------
// Private Chat Tests & Edge Cases
// ------------------------------------------------------------

void testPrivateChat() {

    auto user1 = make_shared<User>(1, "User1");
    auto user2 = make_shared<User>(2, "User2");
    auto user3 = make_shared<User>(3, "User3");

    PrivateChat chat(
        100,
        user1,
        user2);

    assert(chat.getChatId() == 100);
    assert(chat.getParticipants().size() == 2);

    // Cannot add third user
    assert(chat.addUser(user3) == false);

    // Null user cannot be added
    assert(chat.addUser(nullptr) == false);

    // Remove non-existent user should fail
    assert(chat.removeUser(999) == false);

    // Remove existing user
    assert(chat.removeUser(2) == true);
    assert(chat.getParticipants().size() == 1);

    // Exceptions: same user ID in PrivateChat constructor
    bool caughtSameUser = false;
    try {
        PrivateChat invalidChat(101, user1, user1);
    } catch (const invalid_argument&) {
        caughtSameUser = true;
    }
    assert(caughtSameUser);

    // Exceptions: null user in PrivateChat constructor
    bool caughtNullUser = false;
    try {
        PrivateChat invalidChat(102, user1, nullptr);
    } catch (const invalid_argument&) {
        caughtNullUser = true;
    }
    assert(caughtNullUser);

    cout << "testPrivateChat PASSED\n";
}


// ------------------------------------------------------------
// Group Chat Tests & Edge Cases
// ------------------------------------------------------------

void testGroupChat() {

    auto user1 = make_shared<User>(1, "User1");
    auto user2 = make_shared<User>(2, "User2");
    auto user3 = make_shared<User>(3, "User3");
    auto outsider = make_shared<User>(4, "Outsider");

    GroupChat group(
        200,
        "College Friends");

    assert(group.getChatId() == 200);
    assert(group.getGroupName() == "College Friends");

    // Add null user should fail
    assert(!group.addUser(nullptr));

    assert(group.addUser(user1));
    assert(group.addUser(user2));
    assert(group.addUser(user3));

    assert(group.getParticipants().size() == 3);

    // Duplicate user should fail
    assert(!group.addUser(user1));

    // Outsider cannot be made admin
    assert(!group.addAdmin(outsider));

    // Null user cannot be made admin
    assert(!group.addAdmin(nullptr));

    // Add admin
    assert(group.addAdmin(user1));
    assert(group.getAdmins().size() == 1);

    // Duplicate admin should fail
    assert(!group.addAdmin(user1));

    // Remove admin explicitly
    assert(group.removeAdmin(1));
    assert(group.getAdmins().empty());
    assert(!group.removeAdmin(1)); // Already removed

    // Add admin 2, then remove user 2 entirely
    assert(group.addAdmin(user2));
    assert(group.getAdmins().size() == 1);

    // Removing participant also cleans up their admin status
    assert(group.removeUser(2));
    assert(group.getParticipants().size() == 2);
    assert(group.getAdmins().empty());

    // Removing non-existent participant should fail
    assert(!group.removeUser(999));

    cout << "testGroupChat PASSED\n";
}


// ------------------------------------------------------------
// Message Tests
// ------------------------------------------------------------

void testMessages() {

    auto user = make_shared<User>(
        1,
        "Shikshak");

    MessageService service;

    // Text message
    auto textMessage =
        service.createTextMessage(
            1,
            user,
            "Hello");

    assert(textMessage->getMessageId() == 1);
    assert(textMessage->getSender() == user);
    assert(textMessage->getContent() == "Hello");
    assert(textMessage->getStatus() ==
           MessageStatus::SENT);

    textMessage->markDelivered();

    assert(textMessage->getStatus() ==
           MessageStatus::DELIVERED);

    textMessage->markRead();

    assert(textMessage->getStatus() ==
           MessageStatus::READ);

    // Edit
    service.editMessage(
        textMessage,
        "Hello World");

    assert(textMessage->getContent() ==
           "Hello World");

    // Delete
    service.deleteMessage(textMessage);

    assert(textMessage->getContent() ==
           "[Message deleted]");

    cout << "testMessages PASSED\n";
}


// ------------------------------------------------------------
// Media Message Tests
// ------------------------------------------------------------

void testMediaMessage() {

    auto user = make_shared<User>(
        1,
        "Shikshak");

    MessageService service;

    auto mediaMessage =
        service.createMediaMessage(
            1,
            user,
            "photo.jpg",
            MediaType::IMAGE);

    assert(mediaMessage->getMessageId() == 1);
    assert(mediaMessage->getSender() == user);
    assert(mediaMessage->getMediaUrl() ==
           "photo.jpg");

    assert(mediaMessage->getMediaType() ==
           MediaType::IMAGE);

    service.editMessage(
        mediaMessage,
        "new_photo.jpg");

    assert(mediaMessage->getMediaUrl() ==
           "new_photo.jpg");

    service.deleteMessage(mediaMessage);
    assert(mediaMessage->getMediaUrl() ==
           "[Media deleted]");

    cout << "testMediaMessage PASSED\n";
}


// ------------------------------------------------------------
// Chat + Message Integration Test & Null Validation
// ------------------------------------------------------------

void testChatMessaging() {

    auto user1 = make_shared<User>(
        1,
        "User1");

    auto user2 = make_shared<User>(
        2,
        "User2");

    auto chat = make_shared<PrivateChat>(
        100,
        user1,
        user2);

    MessageService service;

    auto message =
        service.createTextMessage(
            1,
            user1,
            "Hello User2");

    service.sendMessage(
        chat,
        message);

    assert(chat->getMessages().size() == 1);
    assert(chat->getMessages()[0]->getMessageId() == 1);

    // Validation: null message in chat->sendMessage
    bool caughtNullMsg = false;
    try {
        chat->sendMessage(nullptr);
    } catch (const invalid_argument&) {
        caughtNullMsg = true;
    }
    assert(caughtNullMsg);

    // Validation: null chat/message in service.sendMessage
    bool caughtNullService = false;
    try {
        service.sendMessage(nullptr, message);
    } catch (const invalid_argument&) {
        caughtNullService = true;
    }
    assert(caughtNullService);

    // Validation: null edit
    bool caughtNullEdit = false;
    try {
        service.editMessage(nullptr, "test");
    } catch (const invalid_argument&) {
        caughtNullEdit = true;
    }
    assert(caughtNullEdit);

    // Validation: null delete
    bool caughtNullDelete = false;
    try {
        service.deleteMessage(nullptr);
    } catch (const invalid_argument&) {
        caughtNullDelete = true;
    }
    assert(caughtNullDelete);

    cout << "testChatMessaging PASSED\n";
}


// ------------------------------------------------------------
// Polymorphism Test (with full assertions)
// ------------------------------------------------------------

void testPolymorphism() {

    auto user = make_shared<User>(
        1,
        "Shikshak");

    MessageService service;

    shared_ptr<Message> text =
        service.createTextMessage(
            1,
            user,
            "Hello");

    shared_ptr<Message> media =
        service.createMediaMessage(
            2,
            user,
            "photo.jpg",
            MediaType::IMAGE);

    // Verify polymorphic display executes cleanly
    text->display();
    media->display();

    // Polymorphic edit
    text->edit("Updated text");
    media->edit("new_photo.jpg");

    auto textDerived = dynamic_pointer_cast<TextMessage>(text);
    assert(textDerived && textDerived->getContent() == "Updated text");

    auto mediaDerived = dynamic_pointer_cast<MediaMessage>(media);
    assert(mediaDerived && mediaDerived->getMediaUrl() == "new_photo.jpg");

    // Polymorphic delete
    text->deleteMessage();
    media->deleteMessage();

    assert(textDerived->getContent() == "[Message deleted]");
    assert(mediaDerived->getMediaUrl() == "[Media deleted]");

    cout << "testPolymorphism PASSED\n";
}


// ------------------------------------------------------------
// Notification Tests
// ------------------------------------------------------------

void testNotification() {

    NotificationService service;
    auto sender = make_shared<User>(1, "SenderUser");
    auto recipient = make_shared<User>(2, "RecipientUser");

    Notification notification(
        NotificationType::MESSAGE,
        "you have a new message",
        recipient,
        sender);

    assert(notification.getType() == NotificationType::MESSAGE);
    assert(notification.getMessage() == "you have a new message");
    assert(notification.getRecipient() == recipient);
    assert(notification.getRecipient()->getName() == "RecipientUser");
    assert(notification.getSender() == sender);
    assert(notification.getSender()->getName() == "SenderUser");
    assert(notification.getTimestamp() != 0);

    service.addNotification(notification);

    assert(service.getNotificationCount() == 1);

    Notification groupNotification(
        NotificationType::GROUP,
        "Added to group");

    assert(groupNotification.getRecipient() == nullptr);
    assert(groupNotification.getSender() == nullptr);
    service.addNotification(groupNotification);

    assert(service.getNotificationCount() == 2);

    cout << "testNotification PASSED\n";
}


// ------------------------------------------------------------
// Backup Tests
// ------------------------------------------------------------

void testBackup() {

    Backup backup(1);

    assert(backup.getBackupId() == 1);

    // No backup initially.
    assert(backup.getLastBackupTime() == 0);
    assert(!backup.restoreBackup());

    // Create backup.
    backup.createBackup();

    assert(backup.getLastBackupTime() != 0);
    assert(backup.restoreBackup());

    cout << "testBackup PASSED\n";
}


// ============================================================
// RUN ALL TESTS
// ============================================================

void runAllTests() {

    cout << "\n========== RUNNING TESTS ==========\n\n";

    testUser();
    testContact();
    testStatus();
    testPrivateChat();
    testGroupChat();
    testMessages();
    testMediaMessage();
    testChatMessaging();
    testPolymorphism();
    testNotification();
    testBackup();

    cout << "\n========== ALL TESTS PASSED ==========\n";
}


// ============================================================
// DEMO
// ============================================================

void runDemo() {

    cout << "\n========== WHATSAPP LLD DEMO ==========\n\n";

    auto shikshak =
        make_shared<User>(1, "Shikshak");

    auto rahul =
        make_shared<User>(2, "Rahul");

    auto aman =
        make_shared<User>(3, "Aman");

    // User
    shikshak->setStatus("Available");

    cout << "User: "
         << shikshak->getName()
         << "\nStatus: "
         << shikshak->getStatus()
         << "\n\n";


    // Private Chat
    auto privateChat =
        make_shared<PrivateChat>(
            101,
            shikshak,
            rahul);

    cout << "Created Private Chat\n";


    // Group Chat
    auto groupChat =
        make_shared<GroupChat>(
            102,
            "NSUT Friends");

    groupChat->addUser(shikshak);
    groupChat->addUser(rahul);
    groupChat->addUser(aman);

    groupChat->addAdmin(shikshak);

    groupChat->displayChat();


    // Messages
    MessageService messageService;

    auto textMessage =
        messageService.createTextMessage(
            1,
            shikshak,
            "Hello Rahul!");

    messageService.sendMessage(
        privateChat,
        textMessage);

    auto mediaMessage =
        messageService.createMediaMessage(
            2,
            rahul,
            "photo.jpg",
            MediaType::IMAGE);

    messageService.sendMessage(
        groupChat,
        mediaMessage);


    cout << "\n--- Private Chat Interaction ---\n";
    cout << "Shikshak sent: \"" << textMessage->getContent() << "\" to Rahul\n";
    cout << "What Rahul received:\n";
    privateChat->displayMessagesForUser(rahul);

    cout << "\n--- Group Chat Interaction (NSUT Friends) ---\n";
    cout << "Rahul sent: " << mediaMessage->getSummary() << " to NSUT Friends\n";
    cout << "What others in the group received:\n";
    for (const auto& member : groupChat->getParticipants()) {
        if (member->getUserId() != rahul->getUserId()) {
            groupChat->displayMessagesForUser(member);
        }
    }

    // Notification
    NotificationService notificationService;

    // Notification for Rahul from Shikshak
    notificationService.addNotification(
        Notification(
            NotificationType::MESSAGE,
            "you have a new message",
            rahul,
            shikshak));

    // Group notifications for other members from Rahul
    for (const auto& member : groupChat->getParticipants()) {
        if (member->getUserId() != rahul->getUserId()) {
            notificationService.addNotification(
                Notification(
                    NotificationType::GROUP,
                    "you have a new message in NSUT Friends",
                    member,
                    rahul));
        }
    }

    cout << "\nNotifications:\n";
    notificationService.sendNotification();


    // Backup
    Backup backup(1);

    backup.createBackup();

    cout << "\nBackup created: "
         << (backup.restoreBackup()
                 ? "Yes"
                 : "No")
         << "\n";


    cout << "\n========== DEMO COMPLETE ==========\n";
}


// ============================================================
// MAIN
// ============================================================

int main() {

    runAllTests();

    runDemo();

    return 0;
}