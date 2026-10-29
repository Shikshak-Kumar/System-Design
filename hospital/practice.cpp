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

enum class Gender{
    MALE,
    FEMALE,
    OTHER
};



enum class BloodGroupType{
    O_POS,
    O_NEG,
    A_POS,
    A_NEG,
};

enum class PaymentType{
    CASH,
    UPI,
    NETBANKING
};

string bloodGroupToString(BloodGroupType type) {
    switch (type) {
        case BloodGroupType::A_POS:
            return "A+";
        case BloodGroupType::A_NEG:
            return "A-";
        case BloodGroupType::O_POS:
            return "O+";
        case BloodGroupType::O_NEG:
            return "O-";
    }

    return "Unknown";
}

class Date{
    public:
        int day;
        int month;
        int year;
        
        explicit Date(const int &day = 1, const int &month = 1, const int &year = 2026):
        day(day),
        month(month),
        year(year){};
        
        string toString() {
            return to_string(day) + "/" + to_string(month) + "/" + to_string(year);
        }
};

class Time{
    private:
        int hour;
        int minute;

    public:
        explicit Time(int hour = 0, int minute = 0) : hour(hour), minute(minute) {}

    int getHour(){
        return hour;
    }

    int getMinute(){
        return minute;
    }

    string toString(){
        return to_string(hour) + (minute<10 ? "0" : "") + to_string(minute);
    }
};

class ContactInfo{
    private:
        string phone;
        string email;
        string address;
    
    public:
        ContactInfo(const string & phone, const string & email, const string & address):
        phone(phone),
        email(email),
        address(address){};
        
        void updateAddress(const string & address){
            this->address = address;
            cout<<"Updated address\n";
        }
        
        string getEmail() const {
            return email;
        }
        
        string getPhone() const {
            return phone;
        }
        
        string getAddress() const {
            return address;
        }
};



class Person{
    protected:
        string name;
        int age;
        Gender gender;
        ContactInfo* contact;
        
    public:
        Person(const string& name, const int & age, Gender &gender, ContactInfo* &contact):
        name(name),
        age(age),
        gender(gender),
        contact(contact){};
        
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

        int getClaimId() const{
            return claimId;
        }

        string getProvider() const {
            return provider;
        }

        string getStatus() const {
            return status;
        }

        double getAmount() const { return amount; }
        
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

class Doctor;
class Patient;

class Appointment{
    private:
        int id;
        Date date;
        Time time;
        Doctor *doctor;
        Patient *patient;
        string reason;
        inline static atomic<int> nextId{1};
    public:
        Appointment(Doctor* doctor, Patient *patient, const string &reason):
        doctor(doctor),
        patient(patient),
        reason(reason){
            id = nextId++;
        }
        
        int getId() const {
            return id;
        }
    
        Date getDate() const {
            return date;
        }
    
        Time getTime() const {
            return time;
        }
    
        const Doctor* getDoctor() const {
            return doctor;
        }
    
        const Patient* getPatient() const {
            return patient;
        }
    
        string getReason() const {
            return reason;
        }
    
        void reschedule(const Date& newDate, const Time& newTime) {
            date = newDate;
            time = newTime;
        }
    
        void printAppointment() const {
            cout << "Appointment ID: " << id << endl;
            cout << "Reason: " << reason << endl;
        }
        
};


// NOTE:

// One shared counter belongs to the entire Patient class,
// not to each Patient object.
// atomic makes incrementing it thread-safe.
// inline static atomic<int> nextId{1};

// When a Patient is created:
// 1. Take the current value of nextId.
// 2. Assign it to this patient's id.
// 3. Increment nextId for the next Patient.
//
// Patient 1 → id = 1, nextId = 2
// Patient 2 → id = 2, nextId = 3
// Patient 3 → id = 3, nextId = 4
// id = nextId++;

class Patient : public Person{
    private:
        int patientId;
        BloodGroupType bloodGroup;
        vector<Claim>insurance;
        vector<unique_ptr<Appointment>>appointments;
        inline static atomic<int> nextId{1};
    
    public:
    
        Patient(const string& name, const int & age, Gender &gender, ContactInfo* &contact,
        BloodGroupType bloodGroup):
        Person(name,age,gender,contact),
        bloodGroup(bloodGroup){
            patientId = nextId++;
        }
        
        
        int getPatientId() const {
            return patientId;
        }
        
        string getBloodGroup() const {
            return bloodGroupToString(bloodGroup);
        }
        
        void addClaim(Claim &claim){
            insurance.push_back(claim);
            cout<<"[Patient]: Claim added\n";
        }
        
        vector<Claim> getClaims(){
            return insurance;
        }
        
        void addAppointment(unique_ptr<Appointment> &appointment){
            appointments.push_back(std::move(appointment));
            cout<<"[Patient]: Appointment added\n";
        }
        
        vector<unique_ptr<Appointment>> &getAppointments(){
            return appointments;
        }
        
        void getInfo() const {
            cout << "----------------------------------------" << endl;
            cout << "Patient ID     : " << patientId << endl;
            cout << "Name           : " << name << endl;
            cout << "Address        : " << (contact ? contact->getAddress() : "N/A") << endl;
            cout << "Blood Group    : " << getBloodGroup() << endl;
            cout << "Contact        : " << (contact ? contact->getPhone() : "N/A") << endl;
            cout << "Insurance      : " << insurance.size() << " claim(s)" << endl;
            for (const auto& c : insurance) {
                cout << "   - Claim #" << c.getClaimId() << " [" << c.getProvider() << "]: ₹"
                     << c.getAmount() << " (" << c.getStatus() << ")" << endl;
            }
            cout << "----------------------------------------" << endl;
        }
        
};

class Staff : public Person{
    protected:
        int id;
        double salary;
        Date doj;
        inline static atomic<int> nextEmpId{1};
    public:
        Staff(const string& name, const int & age, Gender &gender, ContactInfo* &contact,
        const double &salary, Date &doj):
        Person(name,age,gender,contact),
        salary(salary),
        doj(doj){
            id = nextEmpId++;
        };
        
        int getId() const { return id; }
        
        double getSalary() const { return salary; }
        
        Date getDOJ() const { return doj; }
        
        virtual ~Staff() = default;
};

class Doctor : public Staff{
    private:
        string speciality;
        string licenseNo;
    
    public:
        Doctor(const string& name, const int & age, Gender &gender, ContactInfo* &contact,
        const double &salary, Date &doj, const string & speciality, const string & licenseNo):
        Staff(name,age,gender,contact,salary,doj),
        speciality(speciality),
        licenseNo(licenseNo){};
        
        
        string getSpeciality() const { return speciality; }
        string getLicenseNo() const { return licenseNo; }
        
        void isTreating(Patient &patient){
            cout << "\n[Doctor] Dr. " << name << " (" << speciality << ") is treating patient: "
             << patient.getName() << endl;
        }
        
};

class Nurse : public Staff {
    private:
        string shift;
        string ward;
    
    public:
        Nurse(const string& name, const int& age, Gender& gender, ContactInfo*& contact, const double& salary, Date& doj, const string& shift, 
        const string& ward): 
        Staff(name, age, gender, contact, salary, doj), shift(shift), ward(ward) {};
        
        string getShift() const { return shift; }
        string getWard() const { return ward; }
    
        void assist(const Patient& patient) const {
            cout << "\n[Nurse] Nurse " << name << " is assisting " << patient.getName()
                 << " in Ward " << ward << " [" << shift << " Shift]" << endl;
        }
};

// As per need

// class Admin : public Staff {
//     private:
//         string responsibility;
    
//     public:
//         Admin(const string &name, const string &address, ContactInfo* contact, const double &salary, Date doj,
//         string attendant ,const string & responsibility): 
//         Staff(name, address, contact, salary, doj),
//         responsibility(responsibility) {};
    
//         string getResponsibility() const { return responsibility; }
    
//         void manage() const {
//             cout << "[Admin] Admin " << name << " (Attendant: " << attendant
//                  << ") is managing hospital operations." << endl;
//         }
// };

class Room {
    private:
        int roomNo;
        shared_ptr<Patient> patient = nullptr;
    
    public:
        explicit Room(int roomNo) : roomNo(roomNo) {}
        
        int getRoomNo() const {
            return roomNo;
        }
        
        shared_ptr<Patient> getPatient(){
            return patient;
        }
        
        void assigPatient(shared_ptr<Patient> patient){
            this->patient = patient;
        }
        
        void releaseRoom(){
            patient = nullptr;
        }
        
        bool isEmpty(){
            return (patient==nullptr);
        }
};

class Department {
    private:
    int departmentId;
    string departmentName;
    vector<Doctor*> doctors;
    vector<Room*> rooms;
    
    mutex mtx;
    
    counting_semaphore<100> roomSemaphore{0};
    
    public:
    
        Department(const int departmentId, const string departmentName) : departmentName(departmentName), departmentId(departmentId) {}; 
        
        int getDepartmentid() const { return departmentId; }
        string getDepartmentName() const { return departmentName; }   
        
        void addDoctor(Doctor * doctor){
            
            lock_guard<mutex> lock(mtx);
            
            doctors.push_back(doctor);
            
            cout << "[Department] Dr. " << doctor->getName() << " assigned to " << departmentName
             << " department." << endl;
        }
        
        void addRoom(Room* room){
            
            lock_guard<mutex> lock(mtx);
            
            rooms.push_back(room);
            roomSemaphore.release();
            
            cout<<"[Department]: Room "<<room->getRoomNo()<<" added successfully\n";
        }
    
        Room* acquireRoom(shared_ptr<Patient> patient){
            cout << "[Department] " << patient->getName() << " waiting for an available room in " << departmentName
             << "..." << endl;
             
            roomSemaphore.acquire();
            
            lock_guard<mutex> lock(mtx);
            
            if(rooms.empty()){
                roomSemaphore.release();
                return nullptr;
            }
            
            // O(1)
            
            // Room* room = rooms.back();
            
            // rooms.pop_back();
            
            // O(N)
            
            for(auto &room: rooms){
                if(room->isEmpty()){
                    room->assigPatient(patient);
                    cout << "[Department]: " << patient->getName() << " assigned to room " << room->getRoomNo() << endl;
            
                    return room;
                }
            }
            
            cout<<"[Department]: No empty room found\n";
            
            return nullptr;
        }
    
        void releaseRoom(int RoomNo, shared_ptr<Patient> patient) {
            lock_guard<mutex> lock(mtx);
            
            for(auto &room : rooms){
                if(room->getRoomNo() == RoomNo && room->getPatient() == patient){
                    
                    if(room->isEmpty()){
                        cout<<"\n[Room]: Already vacant.\n";
                        return;
                    }
                    
                    room->releaseRoom();
                    roomSemaphore.release();
                    
                    cout<<"\n[Room]: Room is released now.\n";
                    
                    return;
                }
            }
        }
};


class Medicine {
private:
    int id;
    Date expiry;
    string chemical;
    double price;

public:
    Medicine(int id = 0, Date expiry = Date(), string chemical = "", double price = 0.0)
        : id(id), expiry(expiry), chemical(chemical), price(price) {}

    int getId() const { return id; }
    Date getExpiry() const { return expiry; }
    string getChemical() const { return chemical; }
    double getPrice() const { return price; }
};

class Prescription{
    private:
        int prescriptionId;
        Patient* patient;
        Doctor* doctor;
        map<string,Medicine> medicines;
        mutex mtx;
    
    public:
        Prescription(int id, Doctor* doctor, Patient* patient) : prescriptionId(id), doctor(doctor), patient(patient) {};
        
        void addMedicine(string &code, Medicine &medicine){
            lock_guard<mutex> lock(mtx);
            medicines[code] = medicine;
        }
        
        void print(){
            cout << "[Prescription #" << prescriptionId << "] Patient: " << patient->getName() << " | Prescribed by Dr. " << doctor->getName() << endl;
            for (const auto& [code, medicine] : medicines) {
                cout << "   -> Item #" << code << ": " << medicine.getChemical()
                     << " - ₹" << medicine.getPrice() << endl;
            }
        }
    
};



class PaymentStrategy{
    public:
        virtual void pay(double amount) = 0;
        virtual ~PaymentStrategy() = default;
};

class Cash : public PaymentStrategy {
    public: 
        void pay(double amount) override {
            cout << "[Payment:Cash] Cash receipt generated. Amount: ₹" << amount << endl;
        }
};

class Upi : public PaymentStrategy {
public:
    void pay(double amount) override {
        cout << "[Payment:UPI] Instant UPI transfer verified. Amount: ₹" << amount << endl;
    }
};

class NetBanking : public PaymentStrategy {
public:
    void pay(double amount) override {
        cout << "[Payment:NetBanking] NetBanking transaction settled. Amount: ₹" << amount << endl;
    }
};

class PaymentFactory{
    public:
        inline static unique_ptr<PaymentStrategy> create(PaymentType paymentType){
            
            switch(paymentType){
                case PaymentType::CASH:
                    return make_unique<Cash>();
                
                case PaymentType::UPI:
                    return make_unique<Upi>();
                    
                case PaymentType::NETBANKING:
                    return make_unique<NetBanking>();
                default:
                    throw invalid_argument("Unkown payment type");
            }
            
            return nullptr;
                
        }
};

class Payment{
    private:
        int paymentId;
        string status;
        double amount;
        inline static atomic<int> nextId{1};
    public:
    explicit Payment(double amount, string status="PENDING"): amount(amount), status(status) {
        paymentId = nextId++;
    }
    
    int getPaymentId() {
        return paymentId;
    }
    
    string getPaymentStatus(){
        return status;
    }
    
    void processPayment(PaymentType paymentType){
        auto strategy = PaymentFactory::create(paymentType);
        
        if (!strategy) {
            cout << "Invalid payment type\n";
            return;
        }
        
        strategy->pay(amount);
        
        // I wouldn't assume that pay() returning means my local status update is guaranteed. 
        // There can be a crash between the external payment and the database update. 
        // I would persist the payment as PENDING first and use a unique payment ID (here is paymentId) as an idempotency key. 
        // On retry or recovery, I query the payment provider using that ID before initiating another charge.
        
        status = "SUCCESS";
    }
};

class Bill{
    private:
        int id;
        vector<Medicine> medicines;
        inline static atomic<int> nextId{1};
        mutex mtx;
        
        
    public:
        Bill(){
            id = nextId++;
        }
        
        int getBillId(){
            return id;
        }
        
        void addMedicine(const Medicine &medicine){
            lock_guard<mutex> lock(mtx);
            
            medicines.push_back(medicine);
            
            cout << "[Bill #" << id << "] Added medicine: " << medicine.getChemical() << " (₹" << medicine.getPrice() << ")" << endl;
        }
        
        void returnMedicine(const Medicine &medicine){
            lock_guard<mutex> lock(mtx);
            
            for(auto it = medicines.begin(); it != medicines.end(); ++it){
                if(it->getId()==medicine.getId()){
                    medicines.erase(it);
                    cout << "[Bill #" << id << "] Returned medicine: " << medicine.getChemical() << endl;
                    return;
                }
            }
            
            cout << "[Bill #" << id << "] Medicine not found for return." << endl;
        }
        
        double getTotal(){
            lock_guard<mutex> lock(mtx);
            
            double total = 0.0;
            
            for(auto &it: medicines){
                total+=it.getPrice();
            }
            
            cout<<"[Bill] Total: ₹"<<total<<endl;
            
            return total;
        }
        
        void pay(PaymentType paymentType){
            double amount = getTotal();
            
            Payment payment(amount);
            
            payment.processPayment(paymentType);
        }
};








int main(){
    cout << "============================================================" << endl;
    cout << "      HOSPITAL LOW-LEVEL DESIGN (LLD) DEMO TEST             " << endl;
    cout << "============================================================\n" << endl;

    // 1. Contacts
    ContactInfo contactP1("9876543210", "rahul@gmail.com", "Delhi");
    ContactInfo contactP2("9876543211", "aman@gmail.com", "Noida");
    ContactInfo contactDoc("9999999999", "sharma@hospital.com", "New Delhi");
    ContactInfo contactNurse("8888888888", "neha@hospital.com", "New Delhi");

    ContactInfo* cP1 = &contactP1;
    ContactInfo* cP2 = &contactP2;
    ContactInfo* cDoc = &contactDoc;
    ContactInfo* cNurse = &contactNurse;

    Gender male = Gender::MALE;
    Gender female = Gender::FEMALE;

    // 2. Patients & Claims
    auto p1 = make_shared<Patient>("Rahul", 30, male, cP1, BloodGroupType::O_POS);
    auto p2 = make_shared<Patient>("Aman", 25, male, cP2, BloodGroupType::A_POS);

    Claim claim1(501, "Star Health", 4000.0, "Pending");
    claim1.submit();
    claim1.approve();
    p1->addClaim(claim1);

    // 3. Staff: Doctor & Nurse
    Date docDoj(1, 1, 2019);
    Date nurseDoj(1, 6, 2021);

    Doctor doctor("Sharma", 45, male, cDoc, 180000.0, docDoj, "Cardiology", "DOC-CARD-441");
    Nurse nurse("Neha", 28, female, cNurse, 65000.0, nurseDoj, "Morning", "ICU-2");

    nurse.assist(*p1);

    // 4. Department & Rooms
    Department cardiology(10, "Cardiology");
    cardiology.addDoctor(&doctor);

    Room r101(101);
    Room r102(102);
    cardiology.addRoom(&r101);
    cardiology.addRoom(&r102);

    // 5. Appointment
    auto app1 = make_unique<Appointment>(&doctor, p1.get(), "Cardiology Consultation");
    app1->printAppointment();
    p1->addAppointment(app1);

    // 6. Room Acquisition & Treatment
    Room* allocatedRoom = cardiology.acquireRoom(p1);
    doctor.isTreating(*p1);

    // 7. Prescription & Medicines
    Medicine med1(1, Date(31, 12, 2027), "Paracetamol 500mg", 80.0);
    Medicine med2(2, Date(31, 12, 2027), "Atorvastatin 10mg", 320.0);

    Prescription rx(1001, &doctor, p1.get());
    string code1 = "MED1";
    string code2 = "MED2";
    rx.addMedicine(code1, med1);
    rx.addMedicine(code2, med2);
    rx.print();

    // 8. Billing & Payment
    Bill bill;
    bill.addMedicine(med1);
    bill.addMedicine(med2);
    bill.returnMedicine(med1);
    bill.getTotal();
    bill.pay(PaymentType::UPI);

    // 9. Release Room & Print Patient Info
    if (allocatedRoom) {
        cardiology.releaseRoom(allocatedRoom->getRoomNo(), p1);
    }

    p1->getInfo();

    return 0;
}