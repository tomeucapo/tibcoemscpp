#include "stdafx.h"
#include "RequestScheduler.h"
#include "TibcoEMS.h"
#include "AvailRequest.h"
#include "FunctionsCache.h"
#include "AvailResponse.h"
#include "Availability.h"
#include <boost/algorithm/string.hpp>
#include <boost/chrono.hpp>

using namespace avail;

TibcoEMSThread::TibcoEMSThread(PriorityRequestScheduler_TUI &sched ) : scheduler(sched)
{
	connected = false;
	TibcoConnection::ptr_t connection = TibcoConnection::Create("tcp://10.162.63.175:7222");	
	if (connection->Start()) {
		server = SimpleRpcServer::Create(TibcoSession::Create(connection), "ace.bedsbank.requests");
		connected = true;
	}
}

void TibcoEMSThread::ProcessMessage(TibcoMessage::ptr_t &msg) const
{
	string request(msg->Body());
	if ( request.find("AVAIL") == string::npos )
	{
		 server->RespondToMessage(msg, "Incorrect AVAIL");
		 return;
	}
	
	TibcoEMSProcess process(server, msg);
	PriorityRequestInfo req;
	req.priority = 0;

	bool scheduled = scheduler.WaitForTask(req, process);			// Synchronous task
	if ( ! scheduled )
		server->RespondToMessage(msg, "OK 0");
}

void TibcoEMSThread::operator()() const
{
	if (!connected) return;

	while(true)
	{
		TibcoMessage::ptr_t msgRequest;
		if (server->GetNextIncomingMessage(msgRequest))			
			ProcessMessage(msgRequest);		 
	}
}

bool TibcoEMSDaemon::Start(int numThreads)
{
	try {
		for(int t=0;t<numThreads;t++) 
			boost::thread(TibcoEMSThread(scheduler));
	} catch (std::exception &e) {
		Logger::Error(e.what());
		return false;
	}
	return true;
}

/******************************************************************************************************************************************************
 TibcoEMSProcess
 ******************************************************************************************************************************************************/

string TibcoEMSProcess::GetToken(unsigned int index, const vector<string> &tokens) {
	return ( tokens.size() > index ) ? tokens[index] : "";
}

//This method can throw an exception when the incoming request cannot be parsed. This exception should be catched and reported as an error in the response
bool TibcoEMSProcess::ParseRequest(const string &req, AvailRequest &request) {
	vector<string> tokens;
	boost::split(tokens, req, is_any_of("|"), boost::algorithm::token_compress_off);

	//Delete the first item, its the AVAIL string

	if ( ! tokens.empty() )
		tokens.erase(tokens.begin());

	//Check that the number of takens is in allowed range
	if ( tokens.size() < 46 || tokens.size() > 50 )
		return false;

	//Reset the request parameter to an empty request.
	request = AvailRequest();

	request.codidi = tokens[0];

	request.seqtto = boost::lexical_cast<int>(tokens[1]);;

	request.seqsuc = boost::lexical_cast<int>(tokens[2]);;

	if ( ! tokens[3].empty() ) request.paiMer = tokens[3];

	request.codint = tokens[4];

	if ( ! tokens[5].empty() ) request.nomcon = tokens[5];

	if ( ! tokens[6].empty() ) request.codcla = tokens[6];

	if ( ! tokens[7].empty() ) request.indcv = tokens[7][0];

	{
		vector<string> tokFecIni;
		boost::split(tokFecIni, tokens[8], is_any_of("/"), boost::algorithm::token_compress_off);
		if ( tokFecIni.size() == 3 ) {
			struct tm fecIni;
			fecIni.tm_hour = fecIni.tm_min = fecIni.tm_sec = 0;
			fecIni.tm_year = boost::lexical_cast<int>(tokFecIni[2]) - 1900;
			fecIni.tm_mon = boost::lexical_cast<int>(tokFecIni[1]) - 1;
			fecIni.tm_mday = boost::lexical_cast<int>(tokFecIni[0]);
			fecIni.tm_isdst = -1;
			request.fecini = mktime(&fecIni);
		}
	}

	{
		vector<string> tokFecFin;
		boost::split(tokFecFin, tokens[9], is_any_of("/"), boost::algorithm::token_compress_off);
		if ( tokFecFin.size() == 3 ) {
			struct tm fecFin;
			fecFin.tm_hour = fecFin.tm_min = fecFin.tm_sec = 0;
			fecFin.tm_year = boost::lexical_cast<int>(tokFecFin[2]) - 1900;
			fecFin.tm_mon = boost::lexical_cast<int>(tokFecFin[1]) - 1;
			fecFin.tm_mday = boost::lexical_cast<int>(tokFecFin[0]);
			fecFin.tm_isdst = -1;
			request.fecfin = mktime(&fecFin);
		}
	}

	if ( ! tokens[10].empty() ) request.coddes = tokens[10];

	if ( ! tokens[11].empty() )
		request.seqzon = boost::lexical_cast<int>(tokens[11]);

	if ( ! tokens[12].empty() ) request.codcat = tokens[12];

	if ( ! tokens[13].empty() ) request.codreg = tokens[13];

	request.indval = ( tokens[14] == "S" ) ? true : false;

	//Passenger Parsing
	vector<string> ocu, type, ages, isAgent;
	boost::split(ocu, tokens[15], is_any_of("~"), boost::algorithm::token_compress_off);
	boost::split(ages, tokens[16], is_any_of("~"), boost::algorithm::token_compress_off);
	boost::split(type, tokens[17], is_any_of("~"), boost::algorithm::token_compress_off);
	boost::split(isAgent, tokens[18], is_any_of("~"), boost::algorithm::token_compress_off);
	int numDistrib = ocu.size() / 3;    //Se informa para cada distribucion numero de habitacion, de adultos y de niños
	int posPasajero = 0;    //Variable auxiliar para leer datos de cada uno de los pasajeros
	//Antes de parsear la distribucion de pasajeros, se chequea que es correcta
	bool distCorrecta = true;
	if ( ocu.size() % 3 != 0 || numDistrib == 0 || ages.size() != type.size() )
		distCorrecta = false;
	else
	{
		int counter2 = 0;
		int acuPaxes = 0;
		for(int i = 0; i < numDistrib; i++, counter2 += 3) {
			int numHab = boost::lexical_cast<int>(ocu[counter2]), numAdu = boost::lexical_cast<int>(ocu[counter2+1]), numNin = boost::lexical_cast<int>(ocu[counter2+2]), numPaxes = (numAdu + numNin) * numHab;
			acuPaxes += numPaxes;
		}
		if (ages.size() != acuPaxes )
			distCorrecta = false;
	}
	if ( distCorrecta ) 
	{
		int counter = 0;
		for(int i = 0; i < numDistrib; i++, counter += 3) {
			Distribucion distrib;
			int numHab = boost::lexical_cast<int>(ocu[counter]), numAdu = boost::lexical_cast<int>(ocu[counter+1]), numNin = boost::lexical_cast<int>(ocu[counter+2]), numPaxes = (numAdu + numNin) * numHab;
			distrib.NumHab = numHab;
			distrib.NumAdu = numAdu;
			distrib.NumNin = numNin;
			distrib.NumBs = 0;
			vector<Distribucion::Pasajero> pasajeros;
			for(int j = 0; j < numPaxes; j++, posPasajero++) {
				Distribucion::Pasajero pas;
				if ( type[posPasajero].empty() )
					type[posPasajero] = "A";
				switch(type[posPasajero][0])
				{
				case 'A':
					pas.tipo = Distribucion::TipoPasajero::A;
					break;
				case 'N':
					pas.tipo = Distribucion::TipoPasajero::N;
					break;
				}
				pas.edad = boost::lexical_cast<int>(ages[posPasajero]);
				pas.agente = ( isAgent.size() > (unsigned int)posPasajero && isAgent[posPasajero] == "S") ? true : false;
				pasajeros.push_back(pas);
			}
			distrib.SetPasajeros(pasajeros);
			request.distribucion.push_back(distrib);
		}
	}
	else
	{
		//Si la distribución de pasajeros no es correcta, generamos una distribución con edades negativas, lo que provocará que la peticion se descarte al hacerle
		//el check
		Distribucion distrib;
		distrib.NumHab = 1;
		distrib.NumAdu = 1;
		distrib.NumNin = distrib.NumBs = 0;
		Distribucion::Pasajero p;
		p.tipo = Distribucion::TipoPasajero::A;
		

		request.distribucion.push_back(distrib);
	}

	// Código de habitación.
	if ( ! tokens[19].empty() ) request.codhab = tokens[19];

	// Código de característica
	if ( ! tokens[20].empty() ) request.codcar = tokens[20];


	//Lista de tipo de contrato
	if ( ! tokens[21].empty() ) {
		vector<string> tokGrpContrato;
		boost::split(tokGrpContrato, tokens[21], is_any_of("~"), boost::algorithm::token_compress_off);
		foreach(const string &tipContrato, tokGrpContrato)
			request.grpTipoContrato.insert(tipContrato);
	}

	//Lista de Segmentos
	if ( ! tokens[22].empty() ) {
		vector<string> tokGrpSeg;
		boost::split(tokGrpSeg, tokens[22], is_any_of("~"), boost::algorithm::token_compress_off);
		foreach(const string &seg, tokGrpSeg)
			request.grpseg.push_back(atoi(seg.c_str()));
	}

	////Lista de regimenes
	if ( ! tokens[23].empty() ) {
		vector<string> tokReg;
		boost::split(tokReg, tokens[23], is_any_of("~"), boost::algorithm::token_compress_off);
		foreach(const string &reg, tokReg)
			request.grpreg.insert(reg);
	}

	////Lista de Zonas
	if ( ! tokens[24].empty() ) {
		vector<string> zonTok;
		boost::split(zonTok, tokens[24], is_any_of("~"), boost::algorithm::token_compress_off);
		foreach(const string &zon, zonTok)
			request.grpzon.push_back(zon);
	}
	////Lista de Categorias
	if ( ! tokens[25].empty() ) {
		vector<string> catTok;
		boost::split(catTok, tokens[25], is_any_of("~"), boost::algorithm::token_compress_off);
		foreach(const string &cat, catTok)
			request.grpcat.push_back(cat);
	}
	////Lista de Hoteles
	if ( ! tokens[26].empty() ) {
		vector<string> tokHot;
		boost::split(tokHot, tokens[26], is_any_of("~"), boost::algorithm::token_compress_on);
		foreach(const string &hot, tokHot)
			request.grphot.insert(atoi(hot.c_str()));
	}

	if ( ! GetToken(27, tokens).empty() ) request.indDevolverCoste = ( tokens[27][0] == 'S' ) ? true : false;

	if ( ! GetToken(28, tokens).empty() ) request.indTipoContrato = tokens[28][0];

	// PENDIENTE: Estos dos flags no se usan, se podrían eliminar
	request.flags.resize(2);
	if ( ! GetToken(29, tokens).empty() ) request.flags[0] = tokens[29][0];
	if ( ! GetToken(30, tokens).empty() ) request.flags[0] = tokens[30][0];

	if ( ! GetToken(31, tokens).empty() ) request.indgrp = tokens[31][0];

	if ( ! GetToken(32, tokens).empty() ) request.indOnRequest = tokens[32][0];
	
	if ( ! GetToken(33, tokens).empty() ) request.indValDia = tokens[33][0];

	if ( ! GetToken(34, tokens).empty() ) request.indValTto = tokens[34][0];

	if ( ! GetToken(35, tokens).empty() ) request.indFlagPaq = tokens[35][0];

	if ( ! GetToken(36, tokens).empty() ) request.indFlagTar = tokens[36][0];

	if ( ! GetToken(37, tokens).empty() ) request.indFlagNet = tokens[37][0];

	if ( ! GetToken(38, tokens).empty() ) request.indFlagPHo = tokens[38][0];

	if ( ! GetToken(39, tokens).empty() ) request.indFlagSeg = tokens[39][0];

	// CR-6059 Este flag esta sin uso en el codigo, ya que se que ha quitado la logica que lo usaba
	// PENDIENTE: Eliminar campo de la petición
	if ( ! GetToken(40, tokens).empty() ) request.indFlagVia = tokens[40][0];			

	if ( ! GetToken(41, tokens).empty() ) request.indFlagGas = tokens[41][0];

	request.CoreAp = GetToken(42, tokens);

	if ( ! GetToken(43, tokens).empty() ) {
		vector<string> tokArea;
		boost::split(tokArea, tokens[43], is_any_of("~"), boost::algorithm::token_compress_off);
		if ( tokArea.size() == 3) {
			Area area;
			area.Lat = boost::lexical_cast<double>(tokArea[0]);
			area.Lon = boost::lexical_cast<double>(tokArea[1]);
			area.Rad = boost::lexical_cast<double>(tokArea[2]);
			request.ZonDis = area;
		}
	}

	if ( ! GetToken(44, tokens).empty() ) request.minPvp = boost::lexical_cast<double>(tokens[44]);
	
	if ( ! GetToken(45, tokens).empty() ) request.maxPvp = boost::lexical_cast<double>(tokens[45]);

	if ( ! GetToken(46, tokens).empty() ) request.s2CLevel = boost::lexical_cast<int>(tokens[46]);

	if ( ! GetToken(47, tokens).empty() ) request.indFlagErr = boost::lexical_cast<char>(tokens[47]);

	if ( ! GetToken(48, tokens).empty() ) request.indFlagDbg = boost::lexical_cast<char>(tokens[48]);

	return true;
}

string TibcoEMSProcess::AvailabilityResponse(const string &line)
{
	string response;

	Logger::Info("RPC Avail Received : " + line );
	
	try
	{
		AvailRequest req;
		int numLineas = 0;
		if ( ParseRequest(line,req) ) {
			chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
			Availability av(req);
			AvailRequest::ErrorCode returnValue = av.Calcular();
			if ( returnValue == AvailRequest::ErrorCode::TFAST_NOERROR ) {
				size_t size;
				scoped_ptr<char> data(av.GetAvailResponse().SocketResponse(size));
				numLineas = av.GetAvailResponse().NumLineas();
				if ( numLineas > 0 ) {
					response = "OK " + boost::lexical_cast<string>(numLineas) + " |";
					response += string(data.get(), size);
				}
				else
					response = "OK 0";
			}
			else
				response = "ERR " + boost::lexical_cast<string>(returnValue) + " " + AvailRequest::ErrorCodeDescription(returnValue);
			chrono::nanoseconds ns = chrono::high_resolution_clock::now() - start;
			Logger::Info("Avail Processed: " + line + ". Num Lines " + boost::lexical_cast<string>(numLineas) + ". Processed in " + boost::lexical_cast<string>((int)(ns.count() / 1000000.0)) + "ms.");
		}
		else {
			response = "ERR 0 numero de parametros incorrecto";
			Logger::Info("Incorrect Avail Received: " + line);
		}
	}
	catch(...) {
		response = "ERR 0 peticion de avail mal formada";
		Logger::Info("Incorrect Avail Received: " + line);
	}
	
	return response;
}

void TibcoEMSProcess::ExecuteSimple()
{
	server->RespondToMessage(message, AvailabilityResponse(message->Body()));
}

BaseProcess *TibcoEMSProcess::Clone() const
{
	return new TibcoEMSProcess(*this);
}
