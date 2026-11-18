#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;



class MedicalEntity {
protected:
    string name;

public:
    MedicalEntity(string name = "") {
        this->name = name;
    }

    virtual void display() const {
        cout << "Name: " << name << endl;
    }

    virtual ~MedicalEntity() = default;
};



class Medicine : virtual public MedicalEntity {
protected:
    string manufacturer;
    string batchNumber;

public:
    Medicine(string name = "", string manufacturer = "",
            string batch = "")
        : MedicalEntity(name) {
        this->name = name;
        this->manufacturer = manufacturer;
        this->batchNumber = batch;
    }

    string getName() const {
        return name;
    }

    string getManufacturer() const {
        return manufacturer;
    }

    string getBatchNumber() const {
        return batchNumber;
    }

    virtual bool suitableFor(string illness) const {
        return false;
    }

    virtual void display() const override {
        cout << name << " | " << manufacturer
             << " | " << batchNumber << endl;
    }
};


class PrescriptionMedicine : public Medicine {
private:
    vector<string> illnesses;

public:
    PrescriptionMedicine(string name, string manufacturer,
                        string batch, vector<string> illnesses)
        : MedicalEntity(name),
          Medicine(name, manufacturer, batch) {
        this->illnesses = illnesses;
    }

    bool suitableFor(string illness) const override {
        for (const string& disease : illnesses) {
            if (disease == illness)
                return true;
        }
        return false;
    }

    void display() const override {
        cout << name << " | Manufacturer: " << manufacturer
             << " | Batch: " << batchNumber << endl;
    }
};



class DrugInformation : virtual public MedicalEntity {
public:
    void showDrugInfo() {
        cout << "Drug information available." << endl;
    }
};

class SafetyInformation : virtual public MedicalEntity {
public:
    void showSafetyInfo() {
        cout << "Safety information available." << endl;
    }
};

class AdvancedMedicine : public DrugInformation,
                        public SafetyInformation {
public:
    AdvancedMedicine(string name) : MedicalEntity(name) {
        this->name = name;
    }

    void showInformation() {
        showDrugInfo();
        showSafetyInfo();
        display();
    }
};



class MedicineChecker {
public:
    bool checkSuitability(const Medicine& medicine,
                          string illness) {
        return medicine.suitableFor(illness);
    }

    bool checkSuitability(vector<Medicine*>& medicines,
                          string illness) {
        for (Medicine* medicine : medicines) {
            if (!medicine->suitableFor(illness))
                return false;
        }
        return true;
    }
};


class MedicineCount {
private:
    int count;

public:
    MedicineCount(int count = 0) {
        this->count = count;
    }

    MedicineCount operator+(const MedicineCount& other) const {
        return MedicineCount(count + other.count);
    }

    friend ostream& operator<<(ostream& out,
                               const MedicineCount& obj) {
        out << obj.count;
        return out;
    }
};


class MedicinePair {
private:
    string first, second;

public:
    MedicinePair(string a, string b) {
        first = a;
        second = b;
    }

    friend ostream& operator<<(ostream& out,
                               const MedicinePair& obj) {
        out << obj.first << " + " << obj.second;
        return out;
    }
};



class MedicineQuantity {
private:
    int quantity;

public:
    MedicineQuantity(int q = 0) {
        quantity = q;
    }

    int getQuantity() const {
        return quantity;
    }
};



class MedicineScore {
private:
    int score;

public:
    MedicineScore(int s) {
        score = s;
    }

    operator int() const {
        return score;
    }
};



class MedicineRating {
private:
    int rating;

public:
    MedicineRating(int r) {
        rating = r;
    }

    int getRating() const {
        return rating;
    }
};

class MedicineEvaluation {
private:
    int score;

public:
    MedicineEvaluation(const MedicineRating& r) {
        score = r.getRating() * 10;
    }

    int getScore() const {
        return score;
    }
};



template <typename T>
T findMaximum(T a, T b) {
    return (a > b) ? a : b;
}


struct MedicineData {
    string manufacturer;
    string batch;
    vector<string> illnesses;
};

class CounterfeitDetector {
private:
    map<string, MedicineData> database;

public:
    CounterfeitDetector() {
        database["Paracetamol"] = {
            "ABC Pharma", "PAR123",
            {"Fever", "Headache", "Cold"}
        };

        database["Amoxicillin"] = {
            "XYZ Pharma", "AMO456",
            {"Infection", "Bacterial Infection"}
        };

        database["Cetirizine"] = {
            "HealthCare Pharma", "CET789",
            {"Allergy", "Cold"}
        };

        database["Ibuprofen"] = {
            "ABC Pharma", "IBU111",
            {"Pain", "Fever"}
        };
    }

    bool isAuthentic(const Medicine& medicine) {
        auto it = database.find(medicine.getName());

        if (it == database.end())
            return false;

        return it->second.manufacturer ==
                   medicine.getManufacturer()
               &&
               it->second.batch ==
                   medicine.getBatchNumber();
    }

    void check(const Medicine& medicine) {
        cout << medicine.getName() << " -> ";

        if (isAuthentic(medicine))
            cout << "AUTHENTIC";
        else
            cout << "POSSIBLE COUNTERFEIT";

        cout << endl;
    }
};


class InteractionDetector {
public:
    bool hasInteraction(string a, string b) {
        return (a == "Paracetamol" && b == "Ibuprofen")
            || (a == "Ibuprofen" && b == "Paracetamol")
            || (a == "Ibuprofen" && b == "Amoxicillin")
            || (a == "Amoxicillin" && b == "Ibuprofen");
    }

    void check(vector<Medicine*>& medicines) {
        bool found = false;

        for (size_t i = 0; i < medicines.size(); i++) {
            for (size_t j = i + 1; j < medicines.size(); j++) {
                if (hasInteraction(
                        medicines[i]->getName(),
                        medicines[j]->getName())) {

                    MedicinePair pair(
                        medicines[i]->getName(),
                        medicines[j]->getName());

                    cout << "Interaction: " << pair << endl;
                    found = true;
                }
            }
        }

        if (!found)
            cout << "No known interaction found." << endl;
    }
};


int main() {
    string illness;
    int n;

    cout << "====================================\n";
    cout << "          PHARMA GUARD\n";
    cout << "====================================\n";

    cout << "Enter one illness: ";
    getline(cin, illness);

    cout << "Enter number of medicines: ";
    cin >> n;
    cin.ignore();

    if (n <= 0) {
        cout << "Invalid number of medicines.\n";
        return 0;
    }

    map<string, MedicineData> database;

    database["Paracetamol"] = {
        "ABC Pharma", "PAR123",
        {"Fever", "Headache", "Cold"}
    };

    database["Amoxicillin"] = {
        "XYZ Pharma", "AMO456",
        {"Infection", "Bacterial Infection"}
    };

    database["Cetirizine"] = {
        "HealthCare Pharma", "CET789",
        {"Allergy", "Cold"}
    };

    database["Ibuprofen"] = {
        "ABC Pharma", "IBU111",
        {"Pain", "Fever"}
    };

 
    vector<PrescriptionMedicine> medicineObjects;
    medicineObjects.reserve(n);

    for (int i = 0; i < n; i++) {
        string name, manufacturer, batch;

        cout << "\nMedicine " << i + 1 << endl;

        cout << "Name: ";
        getline(cin, name);

        cout << "Manufacturer: ";
        getline(cin, manufacturer);

        cout << "Batch number: ";
        getline(cin, batch);

        auto it = database.find(name);

        if (it == database.end()) {
            cout << "Unknown medicine: " << name << endl;
            return 0;
        }

        medicineObjects.emplace_back(
            name, manufacturer, batch,
            it->second.illnesses);
    }

    vector<Medicine*> medicines;

    for (auto& medicine : medicineObjects) {
        Medicine* basePointer = &medicine;
        PrescriptionMedicine* derivedPointer = &medicine;

        medicines.push_back(basePointer);

        cout << "Loaded: " << derivedPointer->getName() << endl;
    }


    cout << "\n========== SUITABILITY ==========\n";

    MedicineChecker checker;

    for (Medicine* medicine : medicines) {
        cout << medicine->getName() << " -> ";

        if (checker.checkSuitability(*medicine, illness))
            cout << "SUITABLE";
        else
            cout << "NOT SUITABLE";

        cout << endl;
    }

  
    cout << "\nAll medicines suitable? ";

    if (checker.checkSuitability(medicines, illness))
        cout << "YES\n";
    else
        cout << "NO\n";


    cout << "\n========== INTERACTIONS ==========\n";

    InteractionDetector interaction;
    interaction.check(medicines);

   
    cout << "\n========== COUNTERFEIT ==========\n";

    CounterfeitDetector counterfeit;

    for (Medicine* medicine : medicines)
        counterfeit.check(*medicine);

   

    cout << "\n========== OOP DEMONSTRATIONS ==========\n";

   
    MedicineCount a(n), b(1);
    MedicineCount total = a + b;

    cout << "Medicine count + 1 = " << total << endl;

    
    int q = 5;
    MedicineQuantity quantity = q;

    cout << "Basic to Class = "
         << quantity.getQuantity() << endl;

    
    MedicineScore scoreObject(85);
    int score = scoreObject;

    cout << "Class to Basic = " << score << endl;

   
    MedicineRating rating(9);
    MedicineEvaluation evaluation = rating;

    cout << "Class to Class = "
         << evaluation.getScore() << endl;

   
    cout << "Template maximum = "
         << findMaximum(85, 95) << endl;

  
    AdvancedMedicine advanced("Pharma Guard Medicine");

    cout << "\nVirtual Base Class Demonstration:\n";
    advanced.showInformation();


    cout << "\nVirtual Function Demonstration:\n";
    Medicine* ptr = &medicineObjects[0];
    ptr->display();

    cout << "\n========== REPORT COMPLETED ==========\n";

    return 0;
}
