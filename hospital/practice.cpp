#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <semaphore>
#include <chrono>

using namespace std;

class ContactInfo{
    private:
        string phone;
        string email;
        string address;
    
    public:
        ContactInfo(const string & phone, const string & email, const string & address):
        phone(phone),
        email(email),
        address(address):{};
        
        void updateAddress(const string & address){
            this->address = address;
            cout<<"Updated address\n";
        }
        
        string getEmail() const {
            return phone;
        }
        
        string getPhone() const {
            return email;
        }
        
        string getAddress() const {
            return address;
        }
};

enum class Gender{
    MALE,
    FEMALE,
    OTHER
};

class Person{
    private:
        string name;
        int age;
        Gender gender;
        ContactInfo* contact;
        
    public:
        Person(const string& name, const int & age, Gender &gender, ContactInfo* &contact):
        name(name),
        age(age),
        gender(gender),
        contact(contact):{};
        
        string getName() const {
            return name;
        }
        
        int getAge() const{
            return age;
        }
        
        Gender getGender() {
            return gender;
        }
        
        ContactInfo* getContactInfo() {
            return contact;
        }
};

class Patient : public Person{
    
};

class Doctor : public Person{
    
};

class Claim{
    private:
        int claimId;
        string provider;
        double amount;
        string status;

    public:
        Claim(const int &claimId, const string & provider, const double & amount, const string & status):
        claimId(claimId),
        provider(provider),
        amount(amount),
        status(status){};
        
        void submit(){
            status = "Submitted";
        }
        
        void approve(){
            status = "Approved";
        }
        
        void reject(){
            status = "Rejected";
        }

};




int main(){

    return 0;
}