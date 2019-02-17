/* report_service.cc
   Jorge Moreno, 9 de Febrero 2019
   Compile
   $ LD_LIBRARY_PATH=/usr/local/lib && export LD_LIBRARY_PATH && rm -rf report_service && g++ -std=c++11 report_service.cc -o report_service -lpistache -lpthread -v
   Execute
   $./report_service
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

 #define INVOICE_HOST "http://localhost:9090/"

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
    string buildJsonInfo();
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

 class InvoiceDuplicated{
  public:
    InvoiceDuplicated(){}
    InvoiceDuplicated(string id){
      this->insertId(id);
    }
    InvoiceDuplicated(int id){
      this->insertId(id);
    }
    void insertId(int id){
      this->ids.push_back(id);
    }
    void insertId(string id){
      int id_int;
      id_int=stoi(id);
      this->ids.push_back(id_int);
    }
    std::vector<int> getDuplicatedIds(){
      return this->ids;
    }
    int getDuplicateCount(){
      return this->ids.size();
    }
  private:
    std::vector<int> ids;
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

 string Invoice::buildJsonInfo(){
   stringstream root;
   stringstream json_invoice;
   json_invoice << "{ \"id\": " << this->id << ", " \
    << "{ \"razonSocial\": \"" << this->razonSocial << "\", " \
    << "{ \"regimenFiscal\": \"" << this->regimenFiscal << "\", " \
    << "{ \"fecha\": \"" << this->fecha << "\", " \
    << "{ \"lugar\": \"" << this->lugar << "\", " \
    << "{ \"rfc\": \"" << this->rfc << "\", " \
    << "{ \"monto_unitario\": " << this->monto_unitario << ", " \
    << "{ \"monto_final\": " << this->monto_final << " " \
    << "}";
   root << "\"Invoice\": \""<< json_invoice.str() <<"\"";
   std::cout << "json: "<< root.str() << '\n';
   return root.str();
 }

 class AnalysisService{
  public:
    AnalysisService(Address addr):httpEndpoint(std::make_shared<Http::Endpoint>(addr)), desc("Analysis Service API", "0.1"){

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
          .bind(&AnalysisService::infoService, this)
          .response(Http::Code::Ok, "API Info")
          .hide();

      auto analysisV1PathRoute =desc.path("/v1");
      analysisV1PathRoute.route(desc.get("/checkDuplicatedInvoices"),"Check Duplicate Invoices")
            .bind(&AnalysisService::checkDuplicatedInvoices, this)
            .produces(MIME(Application, Json))
            .parameter<Rest::Type::String>("id", "The id of the invoice to delete")
            .response(Http::Code::Ok, "The requested invoice")
            .response(backendErrorResponse);
      analysisV1PathRoute.route(desc.get("/checkAnormalInvoices"),"Check Anormal Invoices by amount")
            .bind(&AnalysisService::checkAnormalInvoices, this)
            .produces(MIME(Application, Json))
            .parameter<Rest::Type::String>("id", "The id of the invoice to delete")
            .response(Http::Code::Ok, "The requested invoice")
            .response(backendErrorResponse);


      analysisV1PathRoute.route(desc.post("/test"), "Test")
            .bind(&AnalysisService::test, this)
            .produces(MIME(Application, Json))
            .consumes(MIME(Application, Json))
            .parameter<Rest::Type::String>("name", "The name of the account to create")
            .response(Http::Code::Ok, "The initial state of the account")
            .response(backendErrorResponse);

    }

    void checkDuplicatedInvoices(const Rest::Request& request, Http::ResponseWriter response) {
        std::vector<Invoice> invoice_vector;
        std::map<string,InvoiceDuplicated> invoice_report;
        stringstream ss;
        std::cout << "func checkDuplicatedInvoices" << '\n';
        invoice_report=this->getAllDuplicatedInvoices();
        if(invoice_report.empty()){
          stringstream response_str;
          response_str << "{\"return\": -1, \"message\": \"Duplicated Invoices Empty!\" }\n";
          response.send(Http::Code::Ok, response_str.str() , MIME(Text, Plain));
          return;
        }else{
          stringstream base_response_str;
          stringstream array_string;

          ss <<invoice_report.size() << "\n";
          std::cout << "Datos Capturados..." << '\n';
          //Realizar Analisis
          int size=invoice_report.size();
          int count=0;
          array_string << "\"InvoicesDuplicates\": [" ;
          for(auto& element:invoice_report){
              if(element.second.getDuplicateCount()>1){
                stringstream folio_base;
                stringstream ids;
                std::vector<int> vector_ids;
                vector_ids=element.second.getDuplicatedIds();
                std::cout << "Folio Duplicado: " << element.first <<'\n';
                ids << "[";
                for (int i = 0; i < vector_ids.size() ; i++) {
                    if(i==(vector_ids.size()-1)){
                      ids<< vector_ids[i] << " ]";
                    }else{
                      ids<< vector_ids[i] << ", ";
                    }
                }
                string final=(count==(size-1))? "":",";
                folio_base<<"{\"folio\": \""<< element.first <<"\", \"ids\": "<< ids.str() << "}" << final;
                std::cout << "Pre Message: " << folio_base.str() << '\n';
                array_string<<folio_base.str();
              }
              count++;
          }
          array_string << "]";
          std::cout << "Array: " << array_string.str() <<'\n';
          base_response_str << "{\"return\": 0, \"message\": \"success\", " << array_string.str() << " }\n";

          response.send(Http::Code::Ok,base_response_str.str() , MIME(Text, Plain));
        }

    }

    void checkAnormalInvoices(const Rest::Request& request, Http::ResponseWriter response) {
      stringstream response_str;
      response_str << "{\"return\": -1, \"message\": \"Not Availabe Now!, Try Later...\" }\n";
      response.send(Http::Code::Ok, response_str.str() , MIME(Text, Plain));
    }

    void test(const Rest::Request& request, Http::ResponseWriter response) {
        string test_response;

        std::cout << "func test" << '\n';
        //test_response=this->getAllDuplicatedInvoices();
        response.send(Http::Code::Ok, test_response, MIME(Text, Plain));
    }

    void infoService(const Rest::Request&, Http::ResponseWriter response){
      std::cout << "func infoService" << '\n';
      response.send(Http::Code::Ok, "Invoice CRUD\n \
      Last Version: 1.0\n \
      Methods:\n \
      v1\\checkDuplicatedInvoices: GET, get all invoices duplicated\n \
      v1\\checkAnormalInvoices: GET, get all anormal invoices by amount\n");
    }

    std::map<string,InvoiceDuplicated> getAllDuplicatedInvoices(){
      string response_body;
      std::vector<Invoice> invoice_vector;
      std::map<string, InvoiceDuplicated> map_duplicates;

      stringstream command;
      Document invoices_json;
      std::cout << "func getAllDuplicatedInvoices" << '\n';

      command << "curl -s " << INVOICE_HOST << "v1/getAll";
      std::cout << "Command: " << command.str() << '\n';

      response_body=getStdoutFromCommand(command.str());


      std::cout << "response: " << response_body << '\n';
      invoices_json.Parse(response_body.c_str());
      if(invoices_json.HasMember("return")){
        int in_return=0;
        std::cout << "return value: " << invoices_json["return"].GetInt() << '\n';
        in_return=invoices_json["return"].GetInt();
        //Obtenemos Invoices
        if(in_return==0 && invoices_json.HasMember("Invoices")){
          const Value& invoices_array = invoices_json["Invoices"];
          for (auto& sigle_invoice : invoices_array.GetArray()){
            const Value& invoice = sigle_invoice["Invoice"];
            Invoice tmp_invoice;
            int id;
            string razonSocial;
            string regimenFiscal;
            string fecha;
            string lugar;
            string rfc;
            string folio;
            double monto_unitario=0.0;
            bool is_duplicated=false;
            map_duplicates.begin();
            std::cout << "Invoice ID: " << invoice["id"].GetInt() << '\n';
            id=invoice["id"].GetInt();
            if(invoice.HasMember("razonSocial")){
              stringstream tmp_json_value;
              tmp_json_value << invoice["razonSocial"].GetString();
              razonSocial=tmp_json_value.str();
            }else{
              std::cout << "ERROR: Falta \"razonSocial\"" << '\n';
              return map_duplicates;//Empty
            }
            if(invoice.HasMember("regimenFiscal")){
              stringstream tmp_json_value;
              tmp_json_value << invoice["regimenFiscal"].GetString();
              regimenFiscal=tmp_json_value.str();
            }else{
              std::cout << "ERROR: Falta \"regimenFiscal\"" << '\n';
              return map_duplicates;//Empty
            }
            if(invoice.HasMember("fecha")){
              stringstream tmp_json_value;
              tmp_json_value << invoice["fecha"].GetString();
              fecha=tmp_json_value.str();
            }else{
              std::cout << "ERROR: Falta \"fecha\"" << '\n';
              return map_duplicates;//Empty
            }
            if(invoice.HasMember("lugar")){
              stringstream tmp_json_value;
              tmp_json_value << invoice["lugar"].GetString();
              lugar=tmp_json_value.str();
            }else{
              std::cout << "ERROR: Falta \"lugar\"" << '\n';
              return map_duplicates;//Empty
            }
            if(invoice.HasMember("rfc")){
              stringstream tmp_json_value;
              tmp_json_value << invoice["rfc"].GetString();
              rfc=tmp_json_value.str();
            }else{
              std::cout << "ERROR: Falta \"rfc\"" << '\n';
              return map_duplicates;//Empty
            }
            if(invoice.HasMember("folio")){
              stringstream tmp_json_value;
              tmp_json_value << invoice["folio"].GetString();
              folio=tmp_json_value.str();
              if(map_duplicates.count(folio)>0){
                std::cout << "Folio duplicado: "<< folio << '\n';
                is_duplicated=true;
                map_duplicates[folio].insertId(id);
              }
            }else{
              std::cout << "ERROR: Falta \"folio\"" << '\n';
              return map_duplicates;//Empty
            }
            if(invoice.HasMember("monto_unitario")){
              monto_unitario=invoice["monto_unitario"].GetDouble();
            }else{
              std::cout << "ERROR: Falta \"monto_unitario\"" << '\n';
              return map_duplicates;//Empty
            }

            tmp_invoice = Invoice();
            tmp_invoice.setId(id);
            tmp_invoice.setRazonSocial(razonSocial);
            tmp_invoice.setRegimenFiscal(regimenFiscal);
            tmp_invoice.setRFC(rfc);
            tmp_invoice.setFecha(fecha);
            tmp_invoice.setLugar(lugar);
            tmp_invoice.setFolio(folio);
            tmp_invoice.setMontoUnitario(monto_unitario);
            invoice_vector.push_back(tmp_invoice);

            if(!is_duplicated){
              InvoiceDuplicated tmp_invoice_duplicated;
              tmp_invoice_duplicated.insertId(id);
              map_duplicates.insert(pair<string,InvoiceDuplicated>(folio,tmp_invoice_duplicated));
            }


          }
          std::cout << "map has: " << map_duplicates.size() << "Elements!!" <<'\n';
          return map_duplicates;
        //Fin obtener Invoices
        }else{
          return map_duplicates;//Empty
        }

        //stringstream tmp_json_value;
        //tmp_json_value << document["razonSocial"].GetString();
        //razonSocial=tmp_json_value.str();
      }else{
        return map_duplicates;
      }

    }

    string getStdoutFromCommand(string cmd) {
      string data;
      FILE * stream;
      const int max_buffer = 256;
      char buffer[max_buffer];
      std::cout << "func getStdoutFromCommand" << '\n';

      cmd.append(" 2>&1");

      stream = popen(cmd.c_str(), "r");
      if (stream) {
      while (!feof(stream))
      if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
      pclose(stream);
      }
      return data;
    }


    std::shared_ptr<Http::Endpoint> httpEndpoint;
    Rest::Description desc;
    Rest::Router router;

 };


int main(int argc, char const *argv[]) {

    int port_server=9091;
    Port port(port_server);
    int thr = 2;

    if (argc >= 2) {
        port = std::stol(argv[1]);

        if (argc == 3)
            thr = std::stol(argv[2]);
    }

    Address addr(Ipv4::any(), port);

    cout << "Analysis Service V 1.0\n Jorge Industries. 2019.\n Running At http://localhost:"<<port_server<< '\n';
    cout << "Cores = " << hardware_concurrency() << endl;
    cout << "Using " << thr << " threads" << endl;

    AnalysisService analysis_serv(addr);
    analysis_serv.init(thr);
    analysis_serv.start();

    analysis_serv.shutdown();

   return 0;
 }
