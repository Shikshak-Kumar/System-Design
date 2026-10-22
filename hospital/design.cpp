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
using namespace std::chrono_literals;

// ============================================================
// Helper Models: Date & DateTime
// ============================================================

struct Date {
    int day;
    int month;
    int year;

    Date(int day = 1, int month = 1, int year = 2026)
        : day(day), month(month), year(year) {}

    string toString() const {
        return to_string(day) + "/" + to_string(month) + "/" + to_string(year);
    }
};

struct DateTime {
    Date date;
    int hour;
    int minute;

    DateTime(Date date = Date(), int hour = 0, int minute = 0)
        : date(date), hour(hour), minute(minute) {}

    string toString() const {
        return date.toString() + " " + to_string(hour) + ":" +
               (minute < 10 ? "0" : "") + to_string(minute);
    }
};

// ============================================================
// ContactInfo
// UML:
//   - phone : string
//   - email : string
//   - address : string
//   + updateAddress(addr : string)
//   + getAddress() : string
// ============================================================

class ContactInfo {
private:
    string phone;
    string email;
    string address;

public:
    ContactInfo(string phone = "", string email = "", string address = "")
        : phone(phone), email(email), address(address) {}

    void updateAddress(const string& addr) { address = addr; }
    string getAddress() const { return address; }
    string getPhone() const { return phone; }
    string getEmail() const { return email; }
};

// ============================================================
// Insurance Claim
// UML:
//   - claimId : int
//   - amount : double
//   - status : string
//   - provider : string
//   + submit()
//   + approve()
//   + reject()
// ============================================================

class Claim {
private:
    int claimId;
    double amount;
    string status;
    string provider;

public:
    Claim(int claimId, double amount, string provider = "National Health", string status = "Pending")
        : claimId(claimId), amount(amount), status(status), provider(provider) {}

    void submit() {
        status = "Submitted";
        cout << "[Claim] Claim #" << claimId << " of ₹" << amount << " submitted to " << provider << endl;
    }

    void approve() {
        status = "Approved";
        cout << "[Claim] Claim #" << claimId << " APPROVED by " << provider << endl;
    }

    void reject() {
        status = "Rejected";
        cout << "[Claim] Claim #" << claimId << " REJECTED by " << provider << endl;
    }

    int getClaimId() const { return claimId; }
    double getAmount() const { return amount; }
    string getStatus() const { return status; }
    string getProvider() const { return provider; }
};

// ============================================================
// Forward Declarations
// ============================================================

class Doctor;
class Patient;
class Appointment;
class Bill;
class Payment;
class PaymentStrategy;

// ============================================================
// Person Hierarchy
// UML:
//   Person:
//     # name : string
//     # address : string
//     # contact : ContactInfo*
// ============================================================

class Person {
protected:
    string name;
    string address;
    ContactInfo* contact;

public:
    Person(string name, string address, ContactInfo* contact)
        : name(name), address(address), contact(contact) {}
    virtual ~Person() = default;

    string getName() const { return name; }
    string getAddress() const { return address; }
    ContactInfo* getContact() const { return contact; }
};

// ============================================================
// Patient
// UML:
//   - id : int
//   - bloodGroup : string
//   - insurance : vector<Claim>
//   + getInfo()
// Composition: Patient "1" *-- "*" Appointment
// Composition: Patient "1" *-- "*" Claim
// ============================================================

class Patient : public Person {
private:
    int id;
    string bloodGroup;
    vector<Claim> insurance;
    vector<unique_ptr<Appointment>> appointments;
    static atomic<int> nextId;

public:
    Patient(string name, string address, ContactInfo* contact, string bloodGroup)
        : Person(name, address, contact), bloodGroup(bloodGroup) {
        id = nextId++;
    }

    int getId() const { return id; }
    string getBloodGroup() const { return bloodGroup; }

    void addClaim(const Claim& claim) {
        insurance.push_back(claim);
    }

    const vector<Claim>& getClaims() const { return insurance; }

    void addAppointment(unique_ptr<Appointment> appointment);
    const vector<unique_ptr<Appointment>>& getAppointments() const { return appointments; }

    void getInfo() const {
        cout << "----------------------------------------" << endl;
        cout << "Patient ID     : " << id << endl;
        cout << "Name           : " << name << endl;
        cout << "Address        : " << address << endl;
        cout << "Blood Group    : " << bloodGroup << endl;
        cout << "Contact        : " << (contact ? contact->getPhone() : "N/A") << endl;
        cout << "Insurance      : " << insurance.size() << " claim(s)" << endl;
        for (const auto& c : insurance) {
            cout << "   - Claim #" << c.getClaimId() << " [" << c.getProvider() << "]: ₹"
                 << c.getAmount() << " (" << c.getStatus() << ")" << endl;
        }
        cout << "----------------------------------------" << endl;
    }
};

atomic<int> Patient::nextId{1};

// ============================================================
// Staff Hierarchy
// UML:
//   Staff:
//     # empId : int
//     # contact : ContactInfo*
//     # salary : double
//     # doj : Date
// ============================================================

class Staff : public Person {
protected:
    int empId;
    ContactInfo* contact;
    double salary;
    Date doj;
    static atomic<int> nextEmpId;

public:
    Staff(string name, string address, ContactInfo* contact, double salary, Date doj)
        : Person(name, address, contact), contact(contact), salary(salary), doj(doj) {
        empId = nextEmpId++;
    }
    virtual ~Staff() = default;

    int getEmpId() const { return empId; }
    double getSalary() const { return salary; }
    Date getDOJ() const { return doj; }
};

atomic<int> Staff::nextEmpId{1};

// ============================================================
// Doctor
// UML:
//   - id : int
//   - speciality : string
//   - licenseNo : string
// ============================================================

class Doctor : public Staff {
private:
    int id;
    string speciality;
    string licenseNo;
    static atomic<int> nextId;

public:
    Doctor(string name, string address, ContactInfo* contact, double salary, Date doj,
           string speciality, string licenseNo)
        : Staff(name, address, contact, salary, doj), speciality(speciality), licenseNo(licenseNo) {
        id = nextId++;
    }

    int getId() const { return id; }
    string getSpeciality() const { return speciality; }
    string getLicenseNo() const { return licenseNo; }

    void treat(const Patient& patient) const {
        cout << "[Doctor] Dr. " << name << " (" << speciality << ") is treating patient: "
             << patient.getName() << endl;
    }
};

atomic<int> Doctor::nextId{1};

// ============================================================
// Nurse
// UML:
//   - id : int
//   - shift : string
//   - ward : string
// ============================================================

class Nurse : public Staff {
private:
    int id;
    string shift;
    string ward;
    static atomic<int> nextId;

public:
    Nurse(string name, string address, ContactInfo* contact, double salary, Date doj,
          string shift, string ward)
        : Staff(name, address, contact, salary, doj), shift(shift), ward(ward) {
        id = nextId++;
    }

    int getId() const { return id; }
    string getShift() const { return shift; }
    string getWard() const { return ward; }

    void assist(const Patient& patient) const {
        cout << "[Nurse] Nurse " << name << " is assisting " << patient.getName()
             << " in Ward " << ward << " [" << shift << " Shift]" << endl;
    }
};

atomic<int> Nurse::nextId{1};

// ============================================================
// Admin
// UML:
//   - id : int
//   - attendant : string
// ============================================================

class Admin : public Staff {
private:
    int id;
    string attendant;
    static atomic<int> nextId;

public:
    Admin(string name, string address, ContactInfo* contact, double salary, Date doj, string attendant)
        : Staff(name, address, contact, salary, doj), attendant(attendant) {
        id = nextId++;
    }

    int getId() const { return id; }
    string getAttendant() const { return attendant; }

    void manage() const {
        cout << "[Admin] Admin " << name << " (Attendant: " << attendant
             << ") is managing hospital operations." << endl;
    }
};

atomic<int> Admin::nextId{1};

// ============================================================
// Hospital Structure: Room & Department
// UML:
//   Department:
//     - did : int
//     - name : string
//     - doctors : vector<Doctor*>
//   Room:
//     - roomNo : string
// Aggregation: Department "1" o-- "1..*" Doctor
// Aggregation: Department o-- Room
// ============================================================

class Room {
private:
    string roomNo;

public:
    explicit Room(string roomNo) : roomNo(roomNo) {}
    string getRoomNo() const { return roomNo; }
};

class Department {
private:
    int did;
    string name;
    vector<Doctor*> doctors;
    vector<Room*> rooms;

    mutex mtx;
    unique_ptr<counting_semaphore<100>> roomSemaphore;

public:
    Department(int did, string name) : did(did), name(name) {}

    int getDid() const { return did; }
    string getName() const { return name; }

    void addDoctor(Doctor* doctor) {
        lock_guard<mutex> lock(mtx);
        doctors.push_back(doctor);
        cout << "[Department] Dr. " << doctor->getName() << " assigned to " << name
             << " department." << endl;
    }

    void addRoom(Room* room) {
        lock_guard<mutex> lock(mtx);
        rooms.push_back(room);
        roomSemaphore = make_unique<counting_semaphore<100>>(static_cast<ptrdiff_t>(rooms.size()));
    }

    Room* acquireRoom(const Patient& patient) {
        cout << "[Room] " << patient.getName() << " waiting for an available room in " << name
             << "..." << endl;
        roomSemaphore->acquire();

        lock_guard<mutex> lock(mtx);
        if (rooms.empty()) {
            roomSemaphore->release();
            return nullptr;
        }

        Room* room = rooms.back();
        rooms.pop_back();
        cout << "[Room] " << patient.getName() << " assigned to room " << room->getRoomNo() << endl;
        return room;
    }

    void releaseRoom(Room* room, const Patient& patient) {
        {
            lock_guard<mutex> lock(mtx);
            rooms.push_back(room);
            cout << "[Room] " << patient.getName() << " checked out. Room " << room->getRoomNo()
                 << " is now vacant." << endl;
        }
        roomSemaphore->release();
    }
};

// ============================================================
// Medicine <<model>>
// UML:
//   - id : int
//   - expiry : Date
//   - chemical : string
//   - price : double
// ============================================================

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

// ============================================================
// Strategy Pattern: PaymentStrategy
// UML:
//   interface PaymentStrategy { + pay(amount : double) }
//   Cash : PaymentStrategy
//   UPI : PaymentStrategy
//   NetBanking : PaymentStrategy
// ============================================================

class PaymentStrategy {
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

class UPI : public PaymentStrategy {
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

// ============================================================
// Payment
// UML:
//   - id : int
//   - status : string
//   - total : double
// Association: Payment --> PaymentStrategy
// ============================================================

class Payment {
private:
    int id;
    string status;
    double total;
    static atomic<int> nextId;

public:
    explicit Payment(double total) : total(total), status("Pending") {
        id = nextId++;
    }

    void process(PaymentStrategy& strategy) {
        cout << "[Payment] Initiating payment ID #" << id << " for ₹" << total << endl;
        strategy.pay(total);
        status = "Success";
        cout << "[Payment] Payment ID #" << id << " status updated to: " << status << endl;
    }

    int getId() const { return id; }
    string getStatus() const { return status; }
    double getTotal() const { return total; }
};

atomic<int> Payment::nextId{1};

// ============================================================
// Bill
// UML:
//   - id : int
//   - medicines : vector<Medicine>
//   + addMed(m : Medicine)
//   + returnMed(m : Medicine)
//   + getTotal() : double
//   + pay(s : PaymentStrategy)
// Aggregation: Bill o-- Medicine
// Association: Bill --> Payment
// ============================================================

class Bill {
private:
    int id;
    vector<Medicine> medicines;
    mutex mtx;
    static atomic<int> nextId;

public:
    Bill() { id = nextId++; }

    int getId() const { return id; }

    void addMed(const Medicine& m) {
        lock_guard<mutex> lock(mtx);
        medicines.push_back(m);
        cout << "[Bill #" << id << "] Added medicine: " << m.getChemical() << " (₹" << m.getPrice() << ")" << endl;
    }

    void returnMed(const Medicine& m) {
        lock_guard<mutex> lock(mtx);
        for (auto it = medicines.begin(); it != medicines.end(); ++it) {
            if (it->getId() == m.getId()) {
                medicines.erase(it);
                cout << "[Bill #" << id << "] Returned medicine: " << m.getChemical() << endl;
                return;
            }
        }
        cout << "[Bill #" << id << "] Medicine not found for return." << endl;
    }

    double getTotal() {
        lock_guard<mutex> lock(mtx);
        double total = 0.0;
        for (const auto& med : medicines) {
            total += med.getPrice();
        }
        return total;
    }

    void pay(PaymentStrategy& strategy) {
        double amount = getTotal();
        Payment payment(amount);
        payment.process(strategy);
    }
};

atomic<int> Bill::nextId{1};

// ============================================================
// Appointment
// UML:
//   - appId : int
//   - doctor : Doctor*
//   - patient : Patient*
//   - dateTime : DateTime
//   - reason : string
//   + getDoctor() : Doctor*
// Associations: Appointment --> Doctor, Appointment --> Bill
// ============================================================

class Appointment {
private:
    int appId;
    Doctor* doctor;
    Patient* patient;
    DateTime dateTime;
    string reason;
    shared_ptr<Bill> bill;
    static atomic<int> nextId;

public:
    Appointment(Doctor* doctor, Patient* patient, DateTime dateTime, string reason)
        : doctor(doctor), patient(patient), dateTime(dateTime), reason(reason), bill(make_shared<Bill>()) {
        appId = nextId++;
    }

    int getId() const { return appId; }
    Doctor* getDoctor() const { return doctor; }
    Patient* getPatient() const { return patient; }
    DateTime getDateTime() const { return dateTime; }
    string getReason() const { return reason; }
    shared_ptr<Bill> getBill() const { return bill; }

    void print() const {
        cout << "[Appointment #" << appId << "] Patient: " << patient->getName()
             << " | Doctor: Dr. " << doctor->getName()
             << " | Time: " << dateTime.toString()
             << " | Reason: " << reason << endl;
    }
};

atomic<int> Appointment::nextId{1};

// Implementation of Patient::addAppointment after Appointment is declared
void Patient::addAppointment(unique_ptr<Appointment> appointment) {
    appointments.push_back(std::move(appointment));
}

// ============================================================
// Prescription
// UML:
//   - id : int
//   - doctor : Doctor*
//   - patient : Patient*
//   - meds : map<code, Medicine>
// Associations: Prescription --> Doctor, Prescription --> Patient, Prescription --> Medicine
// ============================================================

class Prescription {
private:
    int id;
    Doctor* doctor;
    Patient* patient;
    map<int, Medicine> meds;

public:
    Prescription(int id, Doctor* doctor, Patient* patient)
        : id(id), doctor(doctor), patient(patient) {}

    void addMedicine(int code, const Medicine& medicine) {
        meds[code] = medicine;
    }

    void print() const {
        cout << "[Prescription #" << id << "] Patient: " << patient->getName()
             << " | Prescribed by Dr. " << doctor->getName() << endl;
        for (const auto& [code, medicine] : meds) {
            cout << "   -> Item #" << code << ": " << medicine.getChemical()
                 << " - ₹" << medicine.getPrice() << endl;
        }
    }
};

// ============================================================
// Patient Flow Simulation
// ============================================================

void patientFlow(Patient& patient, Doctor& doctor, Department& department, PaymentStrategy& paymentStrategy) {
    cout << "\n>>> [Flow Started] Patient: " << patient.getName() << endl;

    // 1. Create and schedule Appointment
    auto appointment = make_unique<Appointment>(
        &doctor, &patient, DateTime(Date(22, 10, 2026), 10, 30), "Cardiology Consultation"
    );
    appointment->print();

    // 2. Department Room Allocation (Concurrent Control via Semaphore + Mutex)
    Room* room = department.acquireRoom(patient);
    if (!room) {
        cout << "[Alert] No room available for " << patient.getName() << endl;
        return;
    }

    // 3. Treatment by Doctor
    doctor.treat(patient);
    this_thread::sleep_for(400ms);

    // 4. Issue Prescription
    Prescription rx(100 + patient.getId(), &doctor, &patient);
    Medicine med1(1, Date(31, 12, 2027), "Paracetamol 500mg", 80.0);
    Medicine med2(2, Date(31, 12, 2027), "Atorvastatin 10mg", 320.0);

    rx.addMedicine(1, med1);
    rx.addMedicine(2, med2);
    rx.print();

    // 5. Billing & Medicine aggregation
    shared_ptr<Bill> bill = appointment->getBill();
    bill->addMed(med1);
    bill->addMed(med2);
    cout << "[Bill #" << bill->getId() << "] Total payable: ₹" << bill->getTotal() << endl;

    // 6. Release room
    department.releaseRoom(room, patient);

    // 7. Payment via Strategy
    bill->pay(paymentStrategy);

    // Save appointment into Patient record (Patient owns appointments)
    patient.addAppointment(std::move(appointment));
    cout << "<<< [Flow Finished] Patient: " << patient.getName() << "\n" << endl;
}

// ============================================================
// Main Execution
// ============================================================

int main() {
    cout << "============================================================" << endl;
    cout << "      HOSPITAL LOW-LEVEL DESIGN (LLD) SIMULATION            " << endl;
    cout << "============================================================\n" << endl;

    // 1. Contacts
    ContactInfo contactP1("9876543210", "rahul@gmail.com", "Delhi");
    ContactInfo contactP2("9876543211", "aman@gmail.com", "Noida");
    ContactInfo contactP3("9876543212", "rohit@gmail.com", "Gurugram");
    ContactInfo contactStaff("9999999999", "staff@hospital.com", "New Delhi");

    // 2. Patients & Insurance Claims (Patient *-- Claim)
    Patient p1("Rahul", "Delhi", &contactP1, "O+");
    Patient p2("Aman", "Noida", &contactP2, "B+");
    Patient p3("Rohit", "Gurugram", &contactP3, "A+");

    Claim claim1(501, 4000.0, "Star Health");
    claim1.submit();
    claim1.approve();
    p1.addClaim(claim1);

    // 3. Staff: Doctor, Nurse, Admin
    Doctor doctor("Sharma", "Delhi", &contactStaff, 180000.0, Date(1, 1, 2019), "Cardiology", "DOC-CARD-441");
    Nurse nurse("Neha", "Delhi", &contactStaff, 65000.0, Date(1, 1, 2022), "Morning", "ICU-2");
    Admin admin("Amit", "Delhi", &contactStaff, 75000.0, Date(1, 1, 2021), "Front Desk Lead");

    admin.manage();
    nurse.assist(p1);

    // 4. Department & Rooms
    Department cardiology(10, "Cardiology");
    cardiology.addDoctor(&doctor);

    Room r101("R-101");
    Room r102("R-102");
    cardiology.addRoom(&r101);
    cardiology.addRoom(&r102);

    // 5. Payment Strategies
    UPI upiPayment;
    Cash cashPayment;
    NetBanking netBankingPayment;

    // 6. Concurrent Multi-Patient Simulation
    thread t1([&]() { patientFlow(p1, doctor, cardiology, upiPayment); });
    thread t2([&]() { patientFlow(p2, doctor, cardiology, cashPayment); });
    thread t3([&]() { patientFlow(p3, doctor, cardiology, netBankingPayment); });

    t1.join();
    t2.join();
    t3.join();

    // 7. Verify Patient Details & Records
    cout << "============================================================" << endl;
    cout << "                   PATIENT SUMMARY RECORD                   " << endl;
    cout << "============================================================" << endl;
    p1.getInfo();
    cout << "Total appointments recorded for " << p1.getName() << ": " << p1.getAppointments().size() << endl;

    cout << "\nHospital LLD execution completed successfully." << endl;
    return 0;
}