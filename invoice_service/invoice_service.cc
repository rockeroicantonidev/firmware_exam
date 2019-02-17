/* invoice_service.cc
   Jorge Moreno, 9 de Febrero 2019
   Compile
   LD_LIBRARY_PATH=/usr/local/lib && export LD_LIBRARY_PATH && rm -rf invoice_service && g++ -std=c++11 invoice_service.cc -o invoice_service -lpistache -lpthread -v
   Execute ./invoice_service
   Invoice CRUD
*/
 #include <algorithm>
 #include <string>
 #include <vector>
 #include <map>
 #include <iostream>
 #include <pistache/http.h>
 #include <pistache/description.h>
 #include <pistache/endpoint.h>
 #include "rapidjson/document.h"

 #include <pistache/serializer/rapidjson.h> //necesario??

 using namespace std;
 using namespace Pistache;
 using namespace rapidjson;

 namespace Generic{
     void handleReady(const Rest::Request&, Http::ResponseWriter response) {
      response.send(Http::Code::Ok, "1");
  }
 }

 class Invoice{
  public:
    Invoice(){
      std::cout << "Invoice Creado!!" << '\n';
    }
    Invoice(int id, string razonSocial, string regimenFiscal, string fecha, string lugar, string rfc, double monto_unitario){
      this->id=id;
      this->razonSocial=razonSocial;
      this->regimenFiscal=regimenFiscal;
      this->fecha=fecha;
      this->lugar=lugar;
      this->rfc=rfc;
      this->monto_unitario=monto_unitario;

      std::cout << "Invoice Creado, ID: " << this->id << '\n';
    }
    ~Invoice(){}
    void setId(int id){
      this->id=id;
      std::cout << "Invoice ID: " << this->id << '\n';
    }
    void setRazonSocial(string razonSocial){
      this->razonSocial=razonSocial;
    }
    void setRegimenFiscal(string regimenFiscal){
      this->regimenFiscal=regimenFiscal;
    }
    void setFecha(string fecha){
      this->fecha=fecha;
    }
    void setLugar(string lugar){
      this->lugar=lugar;
    }
    void setRFC(string rfc) {
      this->rfc=rfc;
    }
    void setFolio(string folio) {
      this->folio=folio;
    }
    void setMontoUnitario(double monto_unitario){
      this->monto_unitario=monto_unitario;
      this->calcularMontoFinal();
    }
    void deleteThisObject(){
      this->is_deleted=true;
    }
    bool isDeleted(){
      return this->is_deleted;
    }
    double calcularMontoFinal();
    double calcularIVA();
    string buildJsonInfo(bool in_array);
  private:
    int id;
    string razonSocial;
    string regimenFiscal;
    string fecha;
    string lugar;
    string rfc;
    string folio;
    double monto_unitario;
    double monto_final;
    bool is_deleted=false;

 };

 double Invoice::calcularMontoFinal(){
   double tmp;
   tmp=this->monto_unitario+(this->monto_unitario*0.16);
   this->monto_final=tmp;
   std::cout << "Monto Final: " << tmp << '\n';
   return tmp;
 }

 double Invoice::calcularIVA(){
   return this->monto_unitario*0.16;
 }

 string Invoice::buildJsonInfo(bool in_array){
   stringstream root;
   stringstream json_invoice;
   json_invoice << "{ \"id\": " << this->id << ", " \
    << " \"razonSocial\": \"" << this->razonSocial << "\", " \
    << " \"regimenFiscal\": \"" << this->regimenFiscal << "\", " \
    << " \"fecha\": \"" << this->fecha << "\", " \
    << " \"lugar\": \"" << this->lugar << "\", " \
    << " \"rfc\": \"" << this->rfc << "\", " \
    << " \"folio\": \"" << this->folio << "\", " \
    << " \"monto_unitario\": " << this->monto_unitario << ", " \
    << " \"monto_final\": " << this->monto_final << " " \
    << "}";
   if(in_array){
     root << "{ \"Invoice\": "<< json_invoice.str() <<"}";
     std::cout << "json: "<< root.str() << '\n';
     return root.str();
   }else{
     root << "\"Invoice\": "<< json_invoice.str() <<"\n";
     std::cout << "json: "<< root.str() << '\n';
     return root.str();
   }

 }



 class InvoiceService{
  public:
    InvoiceService(Address addr):httpEndpoint(std::make_shared<Http::Endpoint>(addr)), desc("Invoice Service API", "0.1"){

    }
    void init(size_t thr = 2) {
        auto opts = Http::Endpoint::options()
            .threads(thr)
            .flags(Tcp::Options::InstallSignalHandler);
        httpEndpoint->init(opts);
        createDescription();
    }

    void start() {
        router.initFromDescription(desc);

        Rest::Swagger swagger(desc);
        swagger
            .uiPath("/doc")
            .uiDirectory("/home/octal/code/web/swagger-ui-2.1.4/dist")
            .apiPath("/banker-api.json")
            .serializer(&Rest::Serializer::rapidJson)
            .install(router);

        httpEndpoint->setHandler(router.handler());
        httpEndpoint->serve();
    }

    void shutdown() {
        httpEndpoint->shutdown();
    }

    int getInvoiceCount(){
      return this->invoice_count++;
    }

  private:
    void createDescription(){
      desc.info().license("Apache", "http://www.apache.org/licenses/LICENSE-2.0"); //Por convencion

      auto backendErrorResponse = desc.response(Http::Code::Internal_Server_Error, "An error occured with the backend");

      desc.schemes(Rest::Scheme::Http)
          .basePath("/v1")
          .produces(MIME(Application, Json))
          .consumes(MIME(Application, Json));
      desc.route(desc.get("/status"))
          .bind(&Generic::handleReady)
          .response(Http::Code::Ok, "{\"return\": 0, \"message\": \"API READY!\" }\n")
          .hide();
      desc.route(desc.get("/info"))
          .bind(&InvoiceService::infoService, this)
          .response(Http::Code::Ok, "API Info")
          .hide();

      auto invoiceV1PathRoute = desc.path("/v1");
      invoiceV1PathRoute.route(desc.post("/create"), "Create an invoice")
            .bind(&InvoiceService::createInvoice, this)
            .produces(MIME(Application, Json))
            .consumes(MIME(Application, Json))
            .parameter<Rest::Type::String>("json_input", "The data of the invoice to create")
            .response(Http::Code::Ok, "The initial state of the invoice")
            .response(backendErrorResponse);
      invoiceV1PathRoute.route(desc.get("/get/:id"), "Retrieve an invoice")
            .bind(&InvoiceService::getInvoice, this)
            .produces(MIME(Application, Json))
            .parameter<Rest::Type::String>("id", "The id of the invoice to retrieve")
            .response(Http::Code::Ok, "The requested invoice")
            .response(backendErrorResponse);
      invoiceV1PathRoute.route(desc.post("/update/:id"), "Update an invoice")
            .bind(&InvoiceService::updateInvoice, this)
            .produces(MIME(Application, Json))
            .consumes(MIME(Application, Json))
            .parameter<Rest::Type::String>("json_input", "The data of the account to update")
            .parameter<Rest::Type::String>("id", "The id of the invoice to update")
            .response(Http::Code::Ok, "The initial state of the invoice")
            .response(backendErrorResponse);
      invoiceV1PathRoute.route(desc.get("/delete/:id"), "delete an invoice")
            .bind(&InvoiceService::deleteInvoice, this)
            .produces(MIME(Application, Json))
            .parameter<Rest::Type::String>("id", "The id of the invoice to delete")
            .response(Http::Code::Ok, "The requested invoice")
            .response(backendErrorResponse);
      invoiceV1PathRoute.route(desc.get("/getAll"), "Retrieve all invoices")
            .bind(&InvoiceService::getAlInvoices, this)
            .produces(MIME(Application, Json))
            .parameter<Rest::Type::String>("none", "The the all invoices are retrieved")
            .response(Http::Code::Ok, "The requested invoice")
            .response(backendErrorResponse);


      invoiceV1PathRoute.route(desc.post("/test"), "Test")
            .bind(&InvoiceService::test, this)
            .produces(MIME(Application, Json))
            .consumes(MIME(Application, Json))
            .parameter<Rest::Type::String>("name", "The name of the account to create")
            .response(Http::Code::Ok, "The initial state of the account")
            .response(backendErrorResponse);


    }

    void createInvoice(const Rest::Request& request, Http::ResponseWriter response) {
        stringstream ss;
        string message;
        string request_body;
        Document document;
        Invoice tmp_invoice;
        //Elementos del invoice
        string razonSocial;
        string regimenFiscal;
        string fecha;
        string lugar;
        string rfc;
        string folio;
        double monto_unitario=0.0;

        std::cout << "func createInvoice" << '\n';

        //recibimos Json y parceamos
        request_body=request.body();
        document.Parse(request_body.c_str());

        if(document.HasMember("razonSocial")){
          stringstream tmp_json_value;
          tmp_json_value << document["razonSocial"].GetString();
          razonSocial=tmp_json_value.str();
        }else{
          std::cout << "Falta \"razonSocial\"" << '\n';
          ss << "{\"return\": -1, \"message\": \"Error: razonSocial Missing\" }\n";
          message=ss.str();
          response.send(Http::Code::Ok, message);
        }
        if(document.HasMember("regimenFiscal")){
          stringstream tmp_json_value;
          tmp_json_value << document["regimenFiscal"].GetString();
          regimenFiscal=tmp_json_value.str();
        }else{
          std::cout << "Falta \"regimenFiscal\"" << '\n';
          ss << "{\"return\": -1, \"message\": \"Error: regimenFiscal Missing\" }\n";
          message=ss.str();
          response.send(Http::Code::Ok, message);
        }
        if(document.HasMember("fecha")){
          stringstream tmp_json_value;
          tmp_json_value << document["fecha"].GetString();
          fecha=tmp_json_value.str();
        }else{
          std::cout << "Falta \"fecha\"" << '\n';
          ss << "{\"return\": -1, \"message\": \"Error: fecha Missing\" }\n";
          message=ss.str();
          response.send(Http::Code::Ok, message);
        }
        if(document.HasMember("lugar")){
          stringstream tmp_json_value;
          tmp_json_value << document["lugar"].GetString();
          lugar=tmp_json_value.str();
        }else{
          std::cout << "Falta \"lugar\"" << '\n';
          ss << "{\"return\": -1, \"message\": \"Error: lugar Missing\" }\n";
          message=ss.str();
          response.send(Http::Code::Ok, message);
        }
        if(document.HasMember("rfc")){
          stringstream tmp_json_value;
          tmp_json_value << document["rfc"].GetString();
          rfc=tmp_json_value.str();
        }else{
          std::cout << "Falta \"rfc\"" << '\n';
          ss << "{\"return\": -1, \"message\": \"Error: rfc Missing\" }\n";
          message=ss.str();
          response.send(Http::Code::Ok, message);
        }
        if(document.HasMember("folio")){
          stringstream tmp_json_value;
          tmp_json_value << document["folio"].GetString();
          folio=tmp_json_value.str();
        }else{
          std::cout << "Falta \"folio\"" << '\n';
          ss << "{\"return\": -1, \"message\": \"Error: folio Missing\" }\n";
          message=ss.str();
          response.send(Http::Code::Ok, message);
        }
        if(document.HasMember("monto_unitario")){
          monto_unitario=document["monto_unitario"].GetDouble();
        }else{
          std::cout << "Falta \"monto_unitario\"" << '\n';
          ss << "{\"return\": -1, \"message\": \"Error: monto_unitario Missing\" }\n";
          message=ss.str();
          response.send(Http::Code::Ok, message);
        }

        invoice_count++;
        //  Invoice(int id, string razonSocial, string regimenFiscal, string fecha, string lugar, string rfc, double monto_unitario)
        //tmp_invoice=new Invoice(invoice_count,razonSocial,regimenFiscal,fecha,lugar,monto_unitario);
        tmp_invoice = Invoice();
        tmp_invoice.setId(invoice_count);
        tmp_invoice.setRazonSocial(razonSocial);
        tmp_invoice.setRegimenFiscal(regimenFiscal);
        tmp_invoice.setRFC(rfc);
        tmp_invoice.setFecha(fecha);
        tmp_invoice.setLugar(lugar);
        tmp_invoice.setFolio(folio);
        tmp_invoice.setMontoUnitario(monto_unitario);

        string index_count=std::to_string(invoice_count);
        invoice_vector.push_back(tmp_invoice);
        invoice_map.insert(make_pair(index_count,&tmp_invoice));
        ss << "{\"return\": 0, \"invoice_id\": "<<invoice_count << ", \"message\": \"Invoice Created\" }\n";
        message=ss.str();
        response.send(Http::Code::Ok, message);
    }

    void getInvoice(const Rest::Request& request, Http::ResponseWriter response){
      stringstream value;
      string value_str;
      int index=0;
      int size=0;
      Invoice tmp_invoice;
      std::cout << "func getInvoice" << '\n';

      auto id = request.param(":id").as<std::string>();

      std::cout << "Invoice id: " << id << '\n';
      value << id;
      index=stoi(value.str());
      std::cout << "int id: " << index << '\n';
      /*Invoice* tmp_invoice=invoice_map.find(value_str);
      if(tmp_invoice==NULL){
        response.send(Http::Code::Ok, "Not_Found");
      }else{
        response.send(Http::Code::Ok, "TEST TEST");
      }*/
      size=invoice_vector.size();
      //if ((index >= 0 && index < myVector.size()) && (myVector[index] != NULL))
      if(size>0){
        //tmp_invoice_p=&(invoice_vector.at(index));
        tmp_invoice=invoice_vector[index-1];
        if(&tmp_invoice==NULL){
          stringstream response_str;
          response_str << "{\"return\": -1, \"message\": \"not found\" }\n";
          response.send(Http::Code::Ok, response_str.str());
        }else{
          if(tmp_invoice.isDeleted()){
            stringstream response_str;
            response_str << "{\"return\": -1, \"message\": \"not found\" }\n";
            response.send(Http::Code::Ok, response_str.str());
          }else{
            string json_invoice;
            stringstream response_str;
            json_invoice=tmp_invoice.buildJsonInfo(false);
            response_str << "{\"return\": 0, \"message\": \"success\", "<< json_invoice <<" }\n";
            response.send(Http::Code::Ok, response_str.str());
          }

        }
      }else{
        stringstream response_str;
        response_str << "{\"return\": -1, \"message\": \"Invoice Database Empty!\" }\n";
        response.send(Http::Code::Ok, response_str.str());
      }
    }

    void updateInvoice(const Rest::Request& request, Http::ResponseWriter response) {
        stringstream value;
        string value_str;
        int index=0;
        int size=0;
        Invoice tmp_invoice;
        std::cout << "func updateInvoice" << '\n';

        auto id = request.param(":id").as<std::string>();

        std::cout << "Invoice id: " << id << '\n';
        value << id;
        index=stoi(value.str());
        std::cout << "int id: " << index << '\n';

        size=invoice_vector.size();
        //if ((index >= 0 && index < myVector.size()) && (myVector[index] != NULL))
        if(size>0){
          //tmp_invoice_p=&(invoice_vector.at(index));
          tmp_invoice=invoice_vector[index-1];
          if(&tmp_invoice==NULL){
            stringstream response_str;
            response_str << "{\"return\": -1, \"message\": \"not found\" }\n";
            response.send(Http::Code::Ok, response_str.str());
          }else{
            //response.send(Http::Code::Ok, response_str.str());
            if(tmp_invoice.isDeleted()){
              stringstream response_str;
              response_str << "{\"return\": -1, \"message\": \"not found\" }\n";
              response.send(Http::Code::Ok, response_str.str());
              return;
            }

            stringstream ss;
            string message;
            string request_body;
            Document document;
            //Elementos del invoice
            string razonSocial;
            string regimenFiscal;
            string fecha;
            string lugar;
            string rfc;
            double monto_unitario=0.0;

            //recibimos Json y parceamos
            request_body=request.body();
            document.Parse(request_body.c_str());

            if(document.HasMember("razonSocial")){
              stringstream tmp_json_value;
              tmp_json_value << document["razonSocial"].GetString();
              razonSocial=tmp_json_value.str();
              tmp_invoice.setRazonSocial(razonSocial);
            }
            if(document.HasMember("regimenFiscal")){
              stringstream tmp_json_value;
              tmp_json_value << document["regimenFiscal"].GetString();
              regimenFiscal=tmp_json_value.str();
              tmp_invoice.setRegimenFiscal(regimenFiscal);
            }
            if(document.HasMember("fecha")){
              stringstream tmp_json_value;
              tmp_json_value << document["fecha"].GetString();
              fecha=tmp_json_value.str();
              tmp_invoice.setFecha(fecha);
            }
            if(document.HasMember("lugar")){
              stringstream tmp_json_value;
              tmp_json_value << document["lugar"].GetString();
              lugar=tmp_json_value.str();
              tmp_invoice.setLugar(lugar);
            }
            if(document.HasMember("rfc")){
              stringstream tmp_json_value;
              tmp_json_value << document["rfc"].GetString();
              rfc=tmp_json_value.str();
              tmp_invoice.setRFC(rfc);
            }
            if(document.HasMember("monto_unitario")){
              monto_unitario=document["monto_unitario"].GetDouble();
              tmp_invoice.setMontoUnitario(monto_unitario);
            }
            if(document.HasMember("folio")){
              stringstream response_str;
              response_str << "{\"return\": -1, \"message\": \"Folio is a read only field!\" }\n";
              response.send(Http::Code::Ok, response_str.str());
              return;
            }

            //  Invoice(int id, string razonSocial, string regimenFiscal, string fecha, string lugar, string rfc, double monto_unitario)
            //tmp_invoice=new Invoice(invoice_count,razonSocial,regimenFiscal,fecha,lugar,monto_unitario);

            invoice_vector[index-1]=tmp_invoice;
            //string index_count=std::to_string(invoice_count);
            //invoice_vector.push_back(tmp_invoice);
            //invoice_map.insert(make_pair(index_count,&tmp_invoice));
            //ss << "{\"return\": 0, \"invoice_id\": "<<invoice_count << ", \"message\": \"Invoice Created\" }\n";
            //message=ss.str();

            string json_invoice;
            stringstream response_str;
            json_invoice=tmp_invoice.buildJsonInfo(false);
            response_str << "{\"return\": 0, \"message\": \"update success\", "<< json_invoice <<" }\n";

            response.send(Http::Code::Ok, response_str.str());
          }
        }else{
          stringstream response_str;
          response_str << "{\"return\": -1, \"message\": \"Invoice Database Empty!\" }\n";
          response.send(Http::Code::Ok, response_str.str());
        }


    }

    void deleteInvoice(const Rest::Request& request, Http::ResponseWriter response){
      stringstream value;
      string value_str;
      int index=0;
      int size=0;
      Invoice tmp_invoice;
      std::cout << "func deleteInvoice" << '\n';

      auto id = request.param(":id").as<std::string>();

      std::cout << "Invoice id: " << id << '\n';
      value << id;
      index=stoi(value.str());
      std::cout << "int id: " << index << '\n';
      /*Invoice* tmp_invoice=invoice_map.find(value_str);
      if(tmp_invoice==NULL){
        response.send(Http::Code::Ok, "Not_Found");
      }else{
        response.send(Http::Code::Ok, "TEST TEST");
      }*/
      size=invoice_vector.size();
      //if ((index >= 0 && index < myVector.size()) && (myVector[index] != NULL))
      if(size>0){
        //tmp_invoice_p=&(invoice_vector.at(index));
        tmp_invoice=invoice_vector[index-1];
        if(&tmp_invoice==NULL){
          stringstream response_str;
          response_str << "{\"return\": -1, \"message\": \"not found\" }\n";
          response.send(Http::Code::Ok, response_str.str());
        }else{
          if(tmp_invoice.isDeleted()){
            stringstream response_str;
            response_str << "{\"return\": -1, \"message\": \"not found\" }\n";
            response.send(Http::Code::Ok, response_str.str());
            return;
          }
          stringstream response_str;
          tmp_invoice.deleteThisObject();
          invoice_vector[index-1]=tmp_invoice;
          response_str << "{\"return\": 0, \"message\": \"invoice id: "<< index <<" deleted\"}\n";
          response.send(Http::Code::Ok, response_str.str());
        }
      }else{
        stringstream response_str;
        response_str << "{\"return\": -1, \"message\": \"Invoice Database Empty!\" }\n";
        response.send(Http::Code::Ok, response_str.str());
      }
    }

    void getAlInvoices(const Rest::Request& request, Http::ResponseWriter response){
      int size=0;
      Invoice tmp_invoice;
      std::cout << "func getAlInvoices" << '\n';

      size=invoice_vector.size();
      //if ((index >= 0 && index < myVector.size()) && (myVector[index] != NULL))
      if(size>0){
        //tmp_invoice_p=&(invoice_vector.at(index));
        stringstream array_elements;
        stringstream response_str;
        for (int i = 0; i < size; i++) {
          tmp_invoice=invoice_vector[i];
          if(!tmp_invoice.isDeleted()){
            string json_invoice;
            json_invoice=tmp_invoice.buildJsonInfo(true);
            if(i==(size-1)){
              array_elements << json_invoice << "\n";
            }else{
              array_elements << json_invoice << ",\n";
            }
          }else{
            std::cout << "Invoice ID: " << (i+1) << " is deleted!!" << '\n';
          }
        }

        response_str << "{\"return\": 0, \"message\": \"success\", \"Invoices\":["<< array_elements.str() <<"] }\n";
        response.send(Http::Code::Ok, response_str.str());

      }else{
        stringstream response_str;
        response_str << "{\"return\": -1, \"message\": \"Invoice Database Empty!\" }\n";
        response.send(Http::Code::Ok, response_str.str());
      }
    }

    void test(const Rest::Request& request, Http::ResponseWriter response) {
        std::cout << "func test" << '\n';
        response.send(Http::Code::Ok, request.body(), MIME(Text, Plain));
    }

    void infoService(const Rest::Request&, Http::ResponseWriter response){
      std::cout << "func infoService" << '\n';
      response.send(Http::Code::Ok, "Invoice CRUD\n \
      Last Version: 1.0\n \
      Methods:\n \
      v1\\create: POST, create an Invoice\n \
      {\"razonSocial\":\"<string>\", \"regimenFiscal\":\"<string>\", \"fecha\":\"<string>[DD-MM-YYYY]\", \"lugar\":\"<string>\", \"rfc\":\"<string>\",\"folio\":\"<string>\",\"monto_unitario\":<double>} \n \
      v1\\get\\<id>: GET, get an specific Invoice\n \
      v1\\update\\<id>: POST, updated an specific Invoice\n \
      {\"<field_name>\":\"<new value>\", \"<field_name>\":\"<new value>\", ...}\n \
      v1\\delete\\<id>: GET, delete an Invoice\n");
    }

    std::shared_ptr<Http::Endpoint> httpEndpoint;
    Rest::Description desc;
    Rest::Router router;
    std::vector<Invoice> invoice_vector;
    std::map<string, Invoice*> invoice_map;
    int invoice_count=0;

    class Metric {
    public:
        Metric(std::string name, int initialValue = 1)
            : name_(std::move(name))
            , value_(initialValue)
        { }

        int incr(int n = 1) {
            int old = value_;
            value_ += n;
            return old;
        }

        int value() const {
            return value_;
        }

        std::string name() const {
            return name_;
        }
    private:
        std::string name_;
        int value_;
    };

    typedef std::mutex Lock;
    typedef std::lock_guard<Lock> Guard;
    Lock metricsLock;
    std::vector<Metric> metrics;

 };

int main(int argc, char const *argv[]) {

    int port_server=9090;
    Port port(port_server);
    int thr = 2;

    if (argc >= 2) {
        port = std::stol(argv[1]);

        if (argc == 3)
            thr = std::stol(argv[2]);
    }

    Address addr(Ipv4::any(), port);

    cout << "Invoice Service V 1.0\n Jorge Industries. 2019.\n Running At http://localhost:"<<port_server<< '\n';
    cout << "Cores = " << hardware_concurrency() << endl;
    cout << "Using " << thr << " threads" << endl;

    InvoiceService invoice_serv(addr);
    invoice_serv.init(thr);
    invoice_serv.start();

    invoice_serv.shutdown();

   return 0;
 }
