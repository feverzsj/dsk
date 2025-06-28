#include <dsk/util/unordered.hpp>


using namespace dsk;


inline const unstable_unordered_set<uint32_t> ignoredClasses =
{
    27020041,   // sports season
    16510064,   // sporting event
    18608583,   // recurring sporting event
    167170,     // multi-sport event
    13406554,   // sports competition
    51031626,   // sport competition at a multi-sport event
    46190676,   // tennis event
    114466,     // mixed martial arts
    18536800,   // mixed martial arts event
    493386,     // friendly match
    129258010,  // international field hockey match
    17299750,   // snooker tournament
    1366722,    // final
    5595257,    // grand final
    599999,     // semi-final
    11564376,   // quarter-final
    4380246,    // free dance
    4380244,    // free skating
    2031615,    // short program
    1066670,    // tag team
    39825,      // census
    5058975,    // census in Peru
    3887,       // solar eclipse
    11086064,   // total solar eclipse
    28339417,   // hybrid solar eclipse
    5681048,    // partial solar eclipse
    5691927,    // annular solar eclipse
    2668072,    // collection
    7328910,    // art collection
    277511,     // map collection
    110301894,  // picture collection
    9388534,    // archival collection
    60474998,   // digitized collection
    20655472,   // editorial collection
    42939539,   // manuscript collection
    56648173,   // archives
    166118,     // archive
    27032363,   // photo archive
    65651503,   // newspaper clippings archive
    221722,     // bike path
    1248784,    // airport
    644371,     // international airport
    473972,     // protected area
    936257,     // conservation area
    28941950,   // conservation park of South Australia
    61433673,   // textile conservation
    108059873,  // wildlife conservation area
    785979,     // rest area
    30224326,   // themed area
    204832,     // roller coaster
    2389789,    // steel roller coaster
    973560,     // wooden roller coaster
    3080599,    // hybrid roller coaster
    1130875,    // indoor roller coaster
    2516297,    // bobsled roller coaster
    1476801,    // spinning roller coaster
    1414671,    // Wild Mouse roller coaster
    12280,      // bridge
    1068842,    // footbridge
    158218,     // truss bridge
    1429218,    // cantilever bridge
    12570,      // suspension bridge
    537127,     // road bridge
    1825472,    // covered bridge
    976622,     // bridge over valley
    1210334,    // railway bridge
    728937,     // railway line
    2678338,    // railway network
    55678,      // railway stop
    1311958,    // railway tunnel
    55488,      // railway station
    4312270,    // railway station above ground
    357685,     // abandoned railway
    55485,      // dead-end railway station
    15079663,   // rapid transit railway line
    124834336,  // proposed railway line
    4663385,    // former railway station
    20820102,   // rideable miniature railway
    1311670,    // rail infrastructure
    57498564,   // rail track
    14523332,   // backyard railroad
    3038271,    // shortline railroad
    187934,     // monorail
    1349167,    // ground station
    55490,      // through station
    18543139,   // central station
    1187691,    // roadside station in Japan
    15915771,   // funicular station
    2518452,    // experimental station
    200297,     // thermal power station
    1411996,    // Run-of-river hydroelectricity
    15911738,   // hydroelectric power station
    105955498,  // proposed power station
    289,        // television
    511355,     // television studio
    1616075,    // television station
    1261214,    // television special
    2001305,    // television channel
    1254874,    // television network
    5398426,    // television series
    3464665,    // television series season
    21191270,   // television series episode
    117467246,  // animated television series
    15416,      // television program
    120097488,  // television news program
    10676514,   // sports television program
    399811,     // Japanese television drama
    28195059,   // Chinese television drama
    1407245,    // entertainment television program
    746628,     // studio
    672070,     // photographic studio
    62020160,   // fictional resort
    14136353,   // fictional occurrence
    126078851,  // fictional occurrent
    18032959,   // fictional holiday
    126078866,  // fictional temporal entity
    15707521,   // fictional battle
    17198419,   // fictional war
    65562770,   // fictional military operation
    122397289,  // imaginary event
    110799181,  // in-person event
    15900616,   // event sequence
    18340514,   // events in a specific year or time period
    1555508,    // radio program
    14350,      // radio station
    1061197,    // radio network
    14623351,   // radio series
    115730787,  // radio series season
    184356,     // radio telescope
    35273,      // optical telescope
    1179112,    // community radio
    30612,      // clinical trial
    6934595,    // multicenter clinical trial
    2376564,    // interchange
    11313284,   // smart interchange
    46622,      // controlled-access highway
    2126722,    // bicycle highway
    327333,     // government agency
    8010730,    // external agency
    17505024,   // space agency
    194188,     // spaceport
    25956,      // space station
    62447,      // aerodrome
    1542343,    // independent record label
    18127,      // record label
    783794,     // company
    1331793,    // media company
    20819922,   // opera company
    805233,     // ballet company
    1341478,    // energy company
    2442401,    // record company
    2990216,    // mining company
    891723,     // public company
    11812394,   // theater company
    219577,     // holding company
    6500733,    // printing company
    740752,     // transport company
    2085381,    // publishing company
    17102188,   // train operating company
    1762059,    // film production company
    1114515,    // comic publishing company
    2401749,    // telecommunication company
    81529763,   // postcard publishing company
    10689397,   // television production company
    1486290,    // art publisher
    1320047,    // book publisher
    1137109,    // video game publisher
    167037,     // corporation
    17084016,   // nonprofit corporation
    6881511,    // enterprise
    187456,     // bar
    1043639,    // gaybar
    7242780,    // pride house
    51404,      // pride parade
    4127414,    // WorldPride
    112194781,  // Nancy Pride
    7578657,    // Split Pride
    56706985,   // Trans Pride
    4892460,    // Berlin Pride
    12046766,   // Prague Pride
    19926463,   // Brussels Pride
    3289718,    // Toulouse Pride
    58982890,   // Lyon Gay Pride
    51406,      // LGBT pride
    64348974,   // LGBT event
    61851987,   // LGBT magazine
    51167626,   // LGBT nightclub
    62018250,   // LGBT film festival
    29469577,   // LGBT historic place
    17898,      // LGBT rights by country or territory
    118918727,  // The LGBTQ Center of Southern Nevada's Honorarium
    1130199,    // Gay Games
    1573906,    // concert tour
    7315424,    // concert residency
    110880374,  // concert tours of Dua Lipa
    1850936,    // benefit concert
    107736918,  // series of concerts
    3575769,    // zone d'aménagement concerté
    41253,      // movie theater
    821435,     // borough of Berlin
    16751551,   // borough of Brescia
    3927245,    // quarter of Brescia
    202595,     // prefecture of Greece
    1234255,    // regional unit of Greece
    108095628,  // conference edition
    1143604,    // conference proceedings
    2288051,    // party conference
    24406303,   // ELAG Conference
    54805501,   // German Librarians Day Conference
    57659484,   // exhibition hall
    464980,     // exhibition
    124216082,  // exhibition series
    667276,     // art exhibition
    115156410,  // museum exhibition
    29642990,   // fashion exhibition
    10426913,   // permanent exhibition
    29023906,   // temporary exhibition
    18560545,   // periodical exhibition
    936821,     // travelling exhibition
    29643002,   // photography exhibition
    975290,     // International Exhibition
    59861107,   // temporary art exhibition
    154012,     // International Horticultural Exhibition
    11032,      // newspaper
    1110794,    // daily newspaper
    2305295,    // weekly newspaper
    1153191,    // online newspaper
    20850562,   // school newspaper
    738377,     // student newspaper
    54875403,   // research grant
    179049,     // nature reserve
    108059863,  // nature reserve
    158454,     // biosphere reserve
    19656847,   // national nature reserve
    15089606,   // regional nature reserve
    65953608,   // forest managed biological reserve
    65952434,   // forest integral biological reserve
    482994,     // album
    457843,     // album amicorum
    67911299,   // compilation album series
    212198,     // pub
    622425,     // nightclub
    959309,     // coal mine
    16917,      // hospital
    7257872,    // public hospital
    644264,     // children's hospital
    210999,     // psychiatric hospital
    1059324,    // university hospital
    7705274,    // tertiary referral hospital
    64578911,   // former hospital
    41298,      // magazine
    1824338,    // music magazine
    685935,     // trade magazine
    12298619,   // humor magazine
    847906,     // online magazine
    11780435,   // monthly magazine
    1059863,    // women's magazine
    1791899,    // cultural magazine
    1029418,    // children's magazine
    3399338,    // science fiction magazine
    1911105,    // popular science magazine
    15836186,   // television news magazine
    8274,       // manga
    11506025,   // manga award
    21198342,   // manga series
    15296520,   // manga magazine
    127346204,  // design gallery
    1007870,    // art gallery
    383092,     // art academy
    1558054,    // art colony
    2424752,    // product
    43099869,   // dance production
    47467768,   // operatic production
    7777570,    // theatrical production
    9386255,    // production facility
    15501937,   // alqueria
    2072285,    // atelier
    213441,     // shop
    865693,     // gift shop
    2243978,    // record shop
    11315,      // shopping center
    1378975,    // convention center
    1129398,    // fan convention
    111684737,  // British Filk Convention
    7053866,    // North American Science Fiction Convention
    107721263,  // National Puzzlers' League convention
    44377,      // tunnel
    24897103,   // pedestrian tunnel
    39614,      // cemetery
    846659,     // Jewish cemetery
    13883136,   // foundry
    3027462,    // iron foundry
    179700,     // statue
    5635122,    // HM Prison
    40357,      // prison
    83405,      // factory
    4830453,    // business
    10283556,   // motive power depot
    27028153,   // tram depot
    2175765,    // tram stop
    15640053,   // tram system
    15145593,   // tram service
    268592,     // industry
    946725,     // cotton mill
    22698,      // park
    1324355,    // geopark
    53444003,   // UNESCO Global Geopark
    740326,     // water park
    2416723,    // theme park
    46169,      // national park
    22746,      // urban park
    116823488,  // neighbourhood park (Japan)
    194195,     // amusement park
    1144661,    // amusement ride
    1139917,    // haunted attraction
    1934961,    // dark ride
    1415754,    // ghost train
    17232491,   // sleeper train
    30141462,   // rapid train
    15141321,   // train service
    91908084,   // passenger train service
    67454740,   // named passenger train service
    860861,     // sculpture
    2293148,    // sculpture trail
    19479037,   // sculpture series
    11835467,   // abstract sculpture
    2293362,    // group of sculptures
    656720,     // workshop
    27556165,   // workshop
    113493852,  // Trait-Based Approaches to Ocean Life Workshop
    59009655,   // Dagstuhl Perspectives Workshop
    113495119,  // EMODnet Biology workshop
    915466,     // academic year
    737498,     // academic journal
    28134972,   // Academic Training
    40444998,   // academic workshop
    2467461,    // academic department
    4671277,    // academic institution
    2020153,    // academic conference
    47258130,   // academic conference series
    112748789,  // hybrid academic conference
    5633421,    // scientific journal
    1298668,    // science project
    1664720,    // institute
    31855,      // research institute
    50359544,   // Catalonia Institute
    178706,     // institution
    2385804,    // educational institution
    38723,      // higher education institution
    1792623,    // arts educational institution
    115368202,  // defunct government institution
    111594067,  // private educational institution in Catalonia
    3914,       // school
    398141,     // school district
    1321960,    // law school
    9826,       // high school
    4922460,    // Black school
    2143781,    // drama school
    1021290,    // music school
    102201566,  // winter school
    494230,     // medical school
    423208,     // private school
    9842,       // primary school
    111236457,  // Catalonia school
    159334,     // secondary school
    170087,     // folk high school
    6190035,    // Jewish day school
    322563,     // vocational school
    7014642,    // performing arts school
    70206617,   // national primary school
    111244857,  // Institute-school of Catalonia
    115312578,  // Labor Notes Troublemakers School
    428602,     // university-preparatory school
    3918,       // university
    875538,     // public university
    902104,     // private university
    2120466,    // pontifical university
    62078547,   // public research university
    3551519,    // university in Quebec
    15407956,   // university college
    189004,     // college
    1743327,    // church college
    3660535,    // women's college
    1336920,    // community college
    1663017,    // engineering college
    2680291,    // residential college
    21822439,   // further education college
    83620,      // thoroughfare
    25653,      // ferry
    18984099,   // ferry route
    131186,     // choir
    19832486,   // locomotive class
    785745,     // tank locomotive
    20650761,   // tender locomotive
    3461107,    // saddle tank locomotive
    4387609,    // architectural firm
    811979,     // architectural structure
    7307123,    // reference design
    4457957,    // reference architectural design
    3052382,    // fonds
    1021711,    // seaside resort
    2945522,    // naturist resort
    27686,      // hotel
    2880255,    // resort hotel
    11707,      // restaurant
    23812032,   // ramen restaurant
    676586,     // theme restaurant
    18654742,   // pizzeria chain
    177305,     // observation deck 
    15324,      // body of water
    79007,      // street
    34442,      // road
    358078,     // road network
    2354973,    // road tunnel
    1788454,    // road junction
    16939396,   // toll road
    510662,     // ring road
    2145163,    // trunk road
    104637874,  // national road
    1716124,    // national road
    1094196,    // national road in Poland
    12368077,   // secondary road
    1259617,    // Voivodeship road
    909299,     // Swedish county road
    109570442,  // road engineering project
    130748571,  // museum council
    33506,      // museum
    207694,     // art museum
    2327632,    // city museum
    7157526,    // peace museum
    588140,     // science museum
    25183395,   // textile museum
    2772772,    // military museum
    756102,     // open-air museum
    2516357,    // transport museum
    11549301,   // waterworks museum
    4409567,    // prefectural museum
    10624527,   // biographical museum
    12104174,   // ethnographic museum
    36169996,   // volcanological museum
    1970365,    // natural history museum
    245016,     // military base
    695850,     // air base
    43229,      // organization
    104649845,  // film organization
    4438121,    // sports organization
    55097243,   // defunct organization
    110706912,  // Islamic organization
    1156831,    // umbrella organization
    163740,     // nonprofit organization
    7210356,    // political organization
    708676,     // charitable organization
    28456565,   // civic tech organization
    21980538,   // commercial organization
    484652,     // international organization
    79913,      // non-governmental organization
    4502142,    // visual artwork
    20437094,   // installation artwork
    22672348,   // performance artwork
    3153560,    // dance performance
    6942562,    // musical performance
    36784,      // region of France
    3394138,    // police division
    861951,     // police station
    2806212,    // police region
    11948177,   // Sendero Local
    32815,      // mosque
    838948,     // work of art
    47461344,   // written work
    785952,     // public bath
    41176,      // building
    1497364,    // building complex
    63099748,   // hotel building
    294422,     // public building
    1021645,    // office building
    1244442,    // school building
    16970,      // church building
    856584,     // library building
    24354,      // theater building
    13402009,   // apartment building
    19844914,   // university building
    1662011,    // industrial building
    11755880,   // residential building
    57660343,   // performing arts building
    46622804,   // telecommunications building
    2031836,    // Eastern Orthodox church building
    811683,     // proposed building or structure
    19860854,   // destroyed building or structure
    1322402,    // nonbuilding structure
    334383,     // abbey church
    317557,     // parish church
    1129743,    // filial church
    56750657,   // hermitage church
    2496382,    // university church
    217059,     // Unification Church
    102496,     // parish
    932420,     // parish register
    28931760,   // parish of New South Wales
    3947,       // house
    607241,     // clergy house
    39715,      // lighthouse
    7419651,    // open house
    483110,     // stadium
    18142,      // tower block
    15911314,   // association
    23891529,   // honorary consulate
    7843791,    // consulate
    190157,     // transmitter
    17350442,   // venue
    18674739,   // event venue
    8719053,    // music venue
    1076486,    // sports venue
    179076,     // trading venue
    11822917,   // horse racing venue
    15206070,   // popular music venue
    570116,     // tourist attraction
    46124,      // sanatorium
    25550691,   // town hall
    735428,     // town of China
    15127012,   // town in the United States
    15219655,   // civil town of Wisconsin
    5398059,    // Indian reservation of the United States
    612741,     // United States National Forest
    116699470,  // former United States National Forest
    509827,     // United States National Grassland
    97010948,   // state department of transportation of the United States
    1757063,    // lido
    274393,     // bakery
    160645,     // orphanage
    180958,     // faculty
    26703203,   // stolperstein
    850767,     // chairlift
    15284,      // municipality
    768307,     // municipality of Albania
    856076,     // municipality of Finland
    33146843,   // municipality of Catalonia
    2919801,    // municipality of Luxembourg
    117871604,  // former municipality of Greece
    26742250,   // former municipality of Estonia
    685309,     // former municipality of Switzerland
    18663566,   // dissolved municipality of Japan
    28122896,   // rural municipality
    3491915,    // urban-rural municipality
    116457956,  // non-urban municipality in Germany
    991683,     // theoretical municipality
    47928,      // circus
    2301048,    // German special airfield
    759421,     // Naturschutzgebiet
    757292,     // border checkpoint
    7075,       // library
    28564,      // public library
    212805,     // digital library
    385994,     // special library
    856234,     // academic library
    22806,      // national library
    21716051,   // community library
    65768782,   // government library
    160567,     // seismometer
    194356,     // wind farm
    2125157,    // Dutch district court
    31180853,   // community bus
    30014735,   // bus-based transport system
    3240003,    // bus route
    23039057,   // bus model
    811701,     // model series
    788537,     // slot car
    1426271,    // route nationale
    120367839,  // route section in a geographic region
    729477,     // Route Adélie
    184188,     // canton of France
    3677604,    // circle of the Kingdom of Italy
    2977,       // cathedral
    58778,      // system
    1662611,    // IT system
    986008,     // payment system
    12042158,   // fare control system
    1316264,    // Okolia
    330425,     // sanjak
    40558,      // smithy
    819340,     // Bergamt
    97669839,   // ceramics manufactory
    131734,     // brewery
    12518,      // tower
    19850578,   // semaphore tower
    378499,     // semaphore line
    1734300,    // coachbuilder
    30022,      // café
    820477,     // mine
    30129411,   // former local government area of Australia
    108618539,  // Standardised Data on Initiatives
    98550843,   // Standardised Data on Initiatives report (alpha version)
    130262462,  // same-sex marriage in a geographic region
    4801521,    // arts festival
    868557,     // music festival
    1751626,    // theatre festival
    41582469,   // music festival edition
    124312595,  // arts festival edition
    109301270,  // biennale edition
    27968055,   // recurrent event edition
    106594095,  // annual music competition edition
    124312714,  // documenta edition
    4729261,    // All Points West Music & Arts Festival
    2416217,    // theatre troupe
    11183017,   // open-air theatre
    1421268,    // folk theatre
    216107,     // department store
    421253,     // Apple Store
    1195098,    // ward
    670265,     // Eisteddfod
    7900311,    // Urdd National Eisteddfod
    4022,       // river
    484170,     // commune of France
    4476438,    // former district of Kazakhstan
    15687338,   // judicial district of Austria
    3700011,    // district of Indonesia
    690840,     // district of Ethiopia
    515,        // city
    64340309,   // City district of Winterthur
    1093829,    // city in the United States
    494721,     // city of Japan
    21583365,   // city of Estonia
    3199141,    // city of Indonesia
    15221310,   // second-class city
    1193438,    // bullring
    12511,      // stairs
    59772,      // lime kiln
    1498804,    // multifamily residential
    20679540,   // cinema chain
    131263,     // barracks
    3950,       // villa
    532,        // village
    51049922,   // village
    17563638,   // ethnic village
    350895,     // abandoned village
    13100073,   // village of the People's Republic of China
    11446,      // ship
    105999,     // cargo ship
    863915,     // inland port
    702081,     // water board in the Netherlands
    498002,     // aerial tramway
    3257686,    // locality
    15265344,   // broadcaster
    141683,     // broadcast network
    3500933,    // detention basin
    108804811,  // detention house in Japan
    134447,     // nuclear power plant
    63481999,   // Eurovision Song Contest entry
    62584801,   // Eurovision Song Contest selection event
    35718073,   // nation in the Eurovision Song Contest
    54802199,   // nation in the Eurovision competition
    16523578,   // German preliminary rounds for Eurovision Song Contest
    131681,     // reservoir
    1130251,    // NHL entry draft
    431289,     // brand
    2519914,    // brand name
    917562,     // carom billiards
    55655315,   // Carom billiards tournament
    2983893,    // quarter
    12323,      // dam
    38720,      // windmill
    181307,     // lieu-dit
    131290040,  // escuela de la Universidad Nacional Autónoma de México
    132510,     // market
    57607,      // Christmas market
    1585022,    // terminal moraine
    108059862,  // protected landscape
    659909,     // balloon loop
    152810,     // landfill
    22674925,   // former settlement
    101224664,  // settlement house
    47000599,   // Match des Seniors
    2281788,    // public aquarium
    203659,     // teletext
    108679208,  // interview audio recording
    7884789,    // toponym
    21573182,   // natural monument in Germany
    47369034,   // airmobile unit
    30129212,   // artillery unit
    63585458,   // Victoires de la musique ceremony
    121631353,  // curling competition
    596886,     // World Junior Curling Championships
    719627,     // Pacific-Asia Curling Championships
    129292,     // European Mixed Curling Championship
    2005506,    // World Junior Alpine Skiing Championships
    4891477,    // European Weightlifting Federation
    4508911,    // Asian Weightlifting Championships
    25387089,   // African Weightlifting Championships
    54850421,   // Pan American Weightlifting Championships
    3772626,    // European Karate Federation
    12630377,   // European Taekwondo Federation
    676065,     // European Athletic Association
    271805,     // National Collegiate Athletic Association
    739227,     // World Athletics U20 Championships
    1323147,    // World Masters Athletics Championships
    277875,     // Oceania Athletics Championships
    3652179,    // Australian Athletics Championships
    3600404,    // British Athletics Championships
    4509668,    // Russian Athletics Championships
    55632127,   // Russian Indoor Athletics Championships
    10685395,   // Swedish Indoor Athletics Championships
    74173914,   // Norwegian Indoor Athletics Championships
    16961825,   // Arab Junior Athletics Championships
    1417217,    // African Championships in Athletics
    2632641,    // Ibero-American Championships in Athletics
    620932,     // Central American and Caribbean Championships in Athletics
    975128,     // IAAF World Youth Championships in Athletics
    118742853,  // Melanesian Championships in Athletics
    5060322,    // Central American Championships in Athletics
    1125554,    // Confederation of African Athletics
    11316,      // Italian Athletics Federation
    1505454,    // Japan Association of Athletics Federations
    2013417,    // Oceania Athletics Association
    727299,     // Asian Athletics Association
    487066,     // German Athletics Association
    927765,     // National Association of Intercollegiate Athletics
    3092102,    // Polish Athletic Association
    2230227,    // Royal Dutch Athletics Federation
    1124871,    // Atletismo Sudamericano
    3092202,    // Fédération tunisienne d'athlétisme
    189188,     // FA Community Shield
    591412,     // Johan Cruijff Shield
    3540001,    // Driedaagse van Antwerpen
    120966301,  // Malderen Koers
    29224,      // Taça de Portugal
    178743,     // stele
    794754,     // Israel Super Cup
    511086,     // Japanese Super Cup
    937649,     // CAF Super Cup
    219261,     // CAF Champions League
    1824581,    // Liga Leumit
    21193196,   // under-17 sport
    21198428,   // under-18 sport
    21193184,   // under-19 sport
    3547101,    // under-20 sport
    21152744,   // under-21 sport
    14510042,   // under-23 sport
    63891772,   // national championship
    63869675,   // open championship
    2300124,    // United States Women's Open Championship
    58955516,   // junior
    58956013,   // novice
    3994613,    // Pacific Rim Rugby Championship
    4728292,    // All-Japan University Rugby Championship
    1479321,    // French Rugby Federation
    138125,     // Italian Rugby Federation
    3741334,    // Chilean Rugby Federation
    1400291,    // Oceania Rugby
    1788081,    // New Zealand Rugby
    617533,     // Asia Rugby
    3092105,    // Rugby Nederland
    3452920,    // women's rugby union
    16533567,   // Pan-American Rugby Association
    61831177,   // Premiership Rugby Limited
    1124869,    // Sudamérica Rugby
    61329468,   // Lega Italiana Rugby d’Eccellenza
    65814399,   // Celtic Rugby Ltd
    934824,     // National Rugby League
    1125520,    // Rugby Africa
    62970974,   // rugby sevens tournament
    1184953,    // Hong Kong Sevens
    13403399,   // Kirin Holdings
    58733,      // CONMEBOL
    483463,     // Asian Games
    1191392,    // East Asian Games
    1196475,    // South Asian Games
    877484,     // SEA Games
    4583737,    // Central American Games
    1360658,    // Bolivarian Games
    272090,     // Mediterranean Games
    11328791,   // Pacific Mini Games
    1687964,    // Thailand National Games
    16056693,   // National Games of India
    10872326,   // National Games of Taiwan
    1074009,    // National Games of the People's Republic of China
    30158075,   // Champions Challenge mannen
    1741857,    // South American Games
    2665176,    // Islamic Solidarity Games
    117848135,  // CIS Games
    1882557,    // Maccabi World Union
    55690048,   // Union Internationale des Fédérations des Amateurs de Billard
    847053,     // Union Mondiale de Billard
    1851685,    // Skate America
    306124,     // Skate Austria
    6136640,    // Skate Israel
    4049573,    // Skate Canada
    1851677,    // Skate Canada International
    2561953,    // World Skate Europe Rink Hockey
    912933,     // South American U-17 Championship
    2720239,    // Nations Cup
    83145,      // Africa Cup of Nations
    1587816,    // 6 Hours of Silverstone
    171650,     // 12 Hours of Sebring
    11481410,   // 24 Hours of Montjuïc
    123158680,  // 24 Hours of LeMons Australia
    8032,       // Indianapolis 500
    21246076,   // rowing event
    28068544,   // Asian Rowing Federation
    67206435,   // African Rowing Federation
    64495379,   // World Rowing Cup regatta
    6139189,    // World Rowing Junior Championships
    47000346,   // World Rowing Under 23 Regatta
    2953911,    // World Rowing U23 Championships
    80767,      // European Rowing Championships
    42309070,   // European Rowing Junior Championships
    30016890,   // Norwegian rowing championships
    2975537,    // IIHF European Women Championships
    1754117,    // IIHF World Junior Championship
    544199,     // IIHF Asia and Oceania Championship
    1772876,    // IIHF World U18 Championship
    11868450,   // IIHF World U20 Championship Division I
    48324248,   // IIHF World U20 Championship Division II
    25382419,   // IIHF World U20 Championship Division III
    63891738,   // continental championship
    206813,     // Campeonato Brasileiro Série A
    610175,     // Campeonato Brasileiro Série B
    4771007,    // Antarctic field camp
    749622,     // Antarctic research station
    2819161,    // AFL Grand Final
    215380,     // musical group
    965326,     // Confederación Sudamericana de Voleibol
    6851,       // Fédération Internationale de Volleyball
    974919,     // African Volleyball Confederation
    34896236,   // North, Central America and Caribbean Volleyball Confederation
    431852,     // Asian Volleyball Confederation
    6854,       // European Volleyball Confederation
    19622042,   // Belarus Volleyball Federation
    124728098,  // Pan-American Volleyball Union
    9634431,    // Japan Volleyball Association
    306525,     // Österreichischer Volleyball Verband
    85811726,   // U Sports women's volleyball championship
    48843498,   // Asian Women's Volleyball Challenge Cup
    927363,     // Women's Pan-American Volleyball Cup
    3562833,    // women's volleyball at the Asian Games
    96475166,   // Coppa del Mediterraneo
    26806709,   // DDR-Meisterschaften im Schwimmen
    649749,     // Bofrost Cup on Ice
    721747,     // commemorative plaque
    181266,     // Tour of the Basque Country
    1378686,    // Euskal Bizikleta
    11953090,   // Trial de Sant Llorenç
    891243,     // Bol d'Or
    17153085,   // Westercon
    662829,     // Eurocon
    320501,     // Landtag
    1267632,    // raion of Ukraine
    586821,     // Asian TV Cup
    618779,     // award
    38033430,   // class of award
    107547950,  // class of music award
    14936523,   // ironic award
    18328106,   // religion-related award
    15229229,   // theatre award
    2491892,    // environmental award
    378427,     // literary award
    47455701,   // Goya Awards ceremony
    47505518,   // AVN Awards ceremony
    28913516,   // Jussi Awards ceremony
    107659248,  // Amanda Awards Ceremony
    453745,     // Amanda Awards
    1884366,    // magnetic mirror
    3517351,    // TCA Awards
    3406648,    // Producers Guild of America Awards
    1111310,    // Directors Guild of America Award
    5170487,    // Corflu
    121591810,  // gymnastics competition
    676925,     // Swiss Gymnastics Federation
    673212,     // European Gymnastics
    1281845,    // Rhythmic Gymnastics European Championships
    15476659,   // Hungarian Gymnastics Federation
    1671798,    // NHK Trophy
    2988192,    // Finlandia Trophy
    552161,     // Nebelhorn Trophy
    26279902,   // Champions Trophy final game
    390920,     // Baltic Cup
    20019127,   // handball match
    129172778,  // international handball match
    6723123,    // Handball Cup of North Macedonia
    727600,     // Asian Handball Federation
    601860,     // European Handball Federation
    627398,     // Pan-American Team Handball Federation
    2409064,    // African Women's Handball Championship
    17122025,   // Nor.Ca. Women's Handball Championship
    2953981,    // Women's Junior World Handball Championship
    4689963,    // African Women's Youth Handball Championship
    4689962,    // African Women's Junior Handball Championship
    85847280,   // South and Central American Men's Handball Championship
    910796,     // Commonwealth of Independent States Cup
    929905,     // European Judo Championships
    585333,     // Pan American Judo Championships
    16716832,   // Russian Judo Championships
    2954419,    // Asian Judo Championships
    24970150,   // IBSA European Judo Championships
    3606247,    // African Judo Union
    1058658,    // European Judo Union
    222249,     // civil law
    14935528,   // Nikon Indy 300
    286069,     // AACTA Awards
    1529487,    // Logie Awards
    967616,     // World Lacrosse Championship
    2465524,    // Under-19 World Lacrosse Championships
    167071,     // Pekan Olahraga Nasional
    2589882,    // Oceanian Futsal Championship
    719523,     // FIFA U-20 Women's World Cup
    171789,     // FIFA Confederations Cup
    326418,     // FIFA Futsal World Cup
    213070,     // OFC Nations Cup
    528449,     // Caribbean Cup
    189327,     // CONCACAF Gold Cup
    83335,      // CONCACAF Champions Cup
    15784949,   // BBL Champions Cup
    134145,     // Arab Club Champions Cup
    27031964,   // wrestling match
    21156425,   // wrestling event
    838089,     // amateur wrestling
    131359,     // professional wrestling
    17317604,   // professional wrestling event
    1713577,    // World Junior Wrestling Championships
    1256239,    // Asian Wrestling Championships
    2153651,    // African Wrestling Championships
    7565954,    // South American Cross Country Championships
    2954405,    // Asian Cross Country Championships
    1141321,    // European Cross Country Championships
    2954234,    // NACAC Cross Country Championships
    1189918,    // International University Sports Federation
    715608,     // Golden Melody Awards
    863454,     // pier
    3775029,    // Filmfare Awards South
    37049,      // Filmfare Awards
    184349,     // Zee Cine Awards
    1347403,    // FIBA Africa
    1144223,    // FIBA Asia
    427863,     // FIBA Americas
    164836,     // FIBA Europe
    1361246,    // FIBA Oceania
    1388409,    // FIBA Diamond Ball
    101008619,  // rescue excavation
    34876,      // province
    240601,     // province of Algeria
    217691,     // former provinces of Finland
    18431960,   // basketball game
    101003088,  // men's college basketball
    129085076,  // international basketball match
    2297916,    // FIBA Under-17 Basketball World Cup
    1410516,    // FIBA Under-19 Basketball World Cup
    745723,     // FIBA Under-19 Women's Basketball World Cup
    6952237,    // NAIA men's basketball championships
    6952244,    // NAIA Women's Basketball Championships
    4650554,    // ACC men's basketball tournament
    7389625,    // SEC Men's Basketball Tournament
    622521,     // National Basketball Association draft
    11921652,   // Catalan Basketball Federation
    3829440,    // Lega Basket
    60533564,   // British and Irish Lions tour
    1060449,    // Jeux de la Francophonie
    1789476,    // bibliographic database
    1075670,    // Copa Interamericana
    223297,     // Copa Centroamericana
    1130925,    // Copa del Rey de Baloncesto
    485997,     // Supercopa de España
    130332374,  // Supercopa de la Liga MX
    2065736,    // cultural property
    930680,     // Lo Nuestro Awards
    281917,     // Women's PGA Championship
    27650,      // LPGA
    2990946,    // golf tournament
    1187243,    // United States Golf Association
    1363321,    // Professional Golfers' Association of America
    137341,     // NBA All-Star Game
    842793,     // NBA G League
    1813540,    // WNBA draft
    769451,     // WNBA All-Star Game
    10873959,   // China TV Golden Eagle Awards
    353027,     // long-distance cycling route
    53534649,   // cycling team season
    1458017,    // European Cycling Union
    5219706,    // Danish Cycling Federation
    1053117,    // Asian Cycling Confederation
    5894485,    // Hong Kong Cycling Association
    22231119,   // CN
    108525148,  // Canada National Road Cycling Championships - Men ITT
    112679077,  // Curaçao National Road Cycling Championships - Women RR
    112679072,  // Curaçao National Road Cycling Championships - Men ITT
    112679071,  // Curaçao National Road Cycling Championships - Men RR
    112679078,  // Curaçao National Road Cycling Championships - Women ITT
    22680990,   // Namibia National Road Cycling Championships - Men RR
    22681018,   // Namibia National Road Cycling Championships - Men U23 ITT
    22681009,   // Namibia National Road Cycling Championships - Men U23 RR
    22680998,   // Namibia National Road Cycling Championships - Men ITT
    45171831,   // Slovakia National Road Cycling Championships - Women RR
    45171898,   // Slovakia National Road Cycling Championships - Women ITT
    27681898,   // El Salvador National Road Cycling Championships - Men RR
    2244884,    // Dnestr radar
    91001695,   // UNITER Awards Gala
    18397818,   // European Sambo Championships
    5003624,    // memorial
    575759,     // war memorial
    2470247,    // Karl Schäfer Memorial
    2471205,    // Ondrej Nepela Memorial
    871160,     // Deutsche Eislauf-Union
    1479391,    // French Federation of Ice Sports
    1399975,    // Spanish Ice Sports Federation
    582776,     // Italian Ice Sports Federation
    869121,     // European Figure Skating Championships
    11964739,   // Danish Skating Union
    12592598,   // Korea Skating Union
    12361482,   // Estonian Skating Union
    61638765,   // Chinese Taipei Skating Union
    4481872,    // Japan Skating Federation
    17165362,   // long track speed skating
    15872808,   // Asian Single Distance Speed Skating Championships
    2022394,    // Asian Allround Speed Skating Championships
    3492828,    // World All-round qualification speed skating (North America and Oceania)
    16101740,   // Swiss Ice Skating
    16258020,   // British Ice Skating
    16247661,   // Ice Skating Australia
    4071524,    // U.S. Figure Skating
    67641634,   // South African Ice Skating Association
    11992080,   // Norwegian Skating Association
    57612236,   // Icelandic Skating Association
    5100609,    // Chinese Skating Association
    61687988,   // Serbian Skating Association
    67641554,   // South African Figure Skating Association
    10685081,   // Swedish Figure Skating Association
    11894929,   // Finnish Figure Skating Association
    61639015,   // Czech Figure Skating Association
    123916528,  // New Zealand Ice Figure Skating Association
    61686937,   // Slovak Figure Skating Association
    4071518,    // Chinese Figure Skating Association
    5448019,    // Figure Skating Federation of Russia
    11822647,   // Polish Figure Skating Federation
    66654697,   // Mexican Figure Skating Federation
    61688463,   // Ukrainian Figure Skating Federation
    61638444,   // Royal Belgian Figure Skating Federation
    61687761,   // Romanian Skating Federation
    61638882,   // Croatian Skating Federation
    61638567,   // Bulgarian Skating Federation
    123910793,  // Indonesia Ice Skating Federation
    30246230,   // Hungarian National Skating Federation
    61686571,   // Jesenice Figure Skating Club
    4511159,    // Czechoslovak Figure Skating Championships
    1141402,    // Four Continents Figure Skating Championships
    306459,     // Ski Austria
    21140826,   // American football game
    1750919,    // National Football League draft
    258723,     // Bavarian Cup
    341302,     // Czech Lion Awards
    1516352,    // European Combined Events Team Championships
    1154710,    // association football venue
    15991303,   // association football league
    476028,     // association football club
    109623729,  // association football club match
    12166442,   // association football match
    65770283,   // association football final
    17315159,   // international association football match
    9088760,    // friendly association football tournament
    630248,     // Bulgarian Football Union
    318386,     // Russian Football Union
    35572,      // Union of European Football Associations
    497782,     // Football Association of Ireland
    154191,     // German Football Association
    280462,     // Football Association of Republika Srpska
    700929,     // Chinese Taipei Football Association
    579842,     // Japan Football Association
    1475056,    // Iraq Football Association
    9500,       // The Football Association
    722749,     // Uruguayan Football Association
    508376,     // Gibraltar Football Association
    728575,     // Malta Football Association
    213116,     // Royal Dutch Football Association
    337065,     // Football Association of the Czech Republic
    573747,     // Chinese Football Association
    140486,     // Ukrainian Association of Football
    1331838,    // Ligue de Football Professionnel
    309005,     // Portuguese Football Federation
    5049878,    // Castile-León Football Federation
    11389496,   // Japan Football Federation for Members of Society
    244750,     // French Football Federation
    387611,     // Football Federation of Bosnia and Herzegovina
    317705,     // Andorran Football Federation
    20106385,   // Balearic Islands Football Federation
    921559,     // Tunisian Football Federation
    207615,     // Royal Spanish Football Federation
    83276,      // Asian Football Confederation
    244754,     // Brazilian Football Confederation
    168360,     // Confederation of African Football
    308705,     // France Football
    28417718,   // Taiwan Football Premier League
    4729033,    // Japanese Regional Football League Competition
    4729038,    // All Japan Senior Football Championship
    1032414,    // Canadian Soccer Association
    222131,     // United States Soccer Federation
    152665,     // 2. Bundesliga
    476539,     // Coupe de la Ligue
    693797,     // Lumière Awards
    9062425,    // Turia Awards
    653544,     // Trophée des Champions
    3070380,    // Festival du chant de marin
    220505,     // film festival
    27787439,   // film festival edition
    975945,     // Sitges Film Festival
    3550692,    // Metro Manila Film Festival
    6117962,    // Jacksonville Film Festival
    5120812,    // Cinemalaya Philippine Independent Film Festival
    5861094,    // International Short Film Festival of Peru
    448715,     // Netherlands Film Festival
    465682,     // Clermont-Ferrand International Short Film Festival
    21427274,   // Montpellier Mediterranean Film Festival
    1408657,    // Deauville American Film Festival
    3070475,    // Cabourg Film Festival
    3070439,    // British Film Festival Dinard
    3070516,    // Villerupt Italian Film Festival
    3070703,    // La Roche-sur-Yon International Film Festival
    104009207,  // Human Rights Film Festival (Croatia)
    117662095,  // Environmental Film Festival
    118592456,  // AFAN Black & White Party
    107655869,  // group of awards
    646110,     // Algarve Cup
    1408581,    // Festival da Canção
    188075,     // box
    15383322,   // cultural prize
    20102321,   // eleccions al Consell General d'Aran
    11680472,   // Galician Rally Championship
    11448906,   // science award
    1407225,    // television award
    1369077,    // British Academy Television Awards
    1099088,    // National Television Awards
    1364556,    // music award
    106592862,  // annual music awards ceremony
    7695191,    // Tejano Music Awards
    7015517,    // Aotearoa Music Awards
    207601,     // American Music Awards
    16996610,   // iHeartRadio Music Awards
    16968141,   // Radio Disney Music Awards
    20979305,   // Latin American Music Awards
    3087600,    // Billboard Latin Music Awards
    56116950,   // film award category
    18681625,   // Hollywood Film Awards
    594504,     // Asian Film Awards
    223740,     // European Film Awards
    1350697,    // National Film Awards
    8023892,    // Wincon
    111328625,  // Unicon
    30589254,   // Picocon
    7064385,    // Novacon
    5329928,    // Eastercon
    1579590,    // Helvetia Cup
    2210277,    // chess tournament
    3775620,    // Danish Chess Championship
    2375100,    // Croatian Chess Championship
    161981,     // Tata Steel Chess Tournament
    2028330,    // Danish National Cyclo-cross Championships
    703237,     // Echo Music Prize
    47000422,   // Nations Cup
    630018,     // Bambi Award
    204854,     // première
    125451632,  // édition d'une demoparty périodique
    2993090,    // SAT Congress
    15726688,   // Esperanto meeting
    13403102,   // Esperanto congress
    25847,      // Esperanto Youth Week
    12348913,   // Spanish Esperanto Congress
    12345822,   // Brazilian Esperanto Congress
    12561,      // World Congress of Esperanto
    26706,      // International Youth Congress of Esperanto
    25377387,   // Summer Deaflympics
    15990728,   // Winter Deaflympics
    3092943,    // maritime festivals of Brest
    2493694,    // World University Orienteering Championships
    116600516,  // European University Orienteering Championships
    6145647,    // Japan Open
    299425,     // Portugal Open
    1807175,    // Canadian Women's Open
    132731341,  // Moscow Open
    21866936,   // Boston Society of Film Critics Awards
    1638418,    // Los Angeles Film Critics Association
    21522006,   // Dallas-Fort Worth Film Critics Association Awards
    21509286,   // Chicago Film Critics Association Awards
    59645135,   // New York Film Critics Online Awards
    20744362,   // New York Film Critics Circle Awards
    1075347,    // Chlotrudis Awards
    7129610,    // Pan American Race Walking Cup
    910489,     // Premios TVyNovelas
    1892098,    // Petit Le Mans
    1785973,    // Chevron Championship
    1852166,    // WGC Match Play
    494588,     // Young Artist Awards
    13019490,   // Star Entertainment Awards
    2906822,    // Blockbuster Entertainment Awards
    7032808,    // Nigeria Entertainment Awards
    268200,     // Screen Actors Guild Award
    921469,     // Latin Grammy Awards
    501058,     // People's Choice Awards
    7585305,    // Critics' Choice Movie Award
    65043330,   // Critics' Choice Real TV Awards
    96197132,   // Nickelodeon's Kids' Choice Sports Awards
    3183530,    // Swedish Sports Awards
    4377069,    // Muz-TV Awards
    18640746,   // Primetime Emmy Award annual ceremony
    6843019,    // Midsouth Emmy Awards
    10354837,   // International Emmy Award
    3494164,    // Sports Emmy Awards
    7277824,    // RTHK Top 10 Gold Songs Awards
    157894,     // AFC Asian Cup
    482313,     // Amílcar Cabral Cup
    2528031,    // Africa Cup
    658986,     // CECAFA Cup
    1779329,    // Gullruten
    63040754,   // multiple unit class
    1439559,    // Fotbollsgalan
    1377017,    // European Table Tennis Union
    1499149,    // European Table Tennis Championships
    1153025,    // wheelchair tennis
    3831285,    // Levene Gouldin & Thompson Tennis Challenger
    1792571,    // art prize
    896958,     // Landesgartenschau
    2144108,    // FIL European Luge Championships
    1504013,    // list of FIL European Luge Champions
    217000,     // Deutsche Tourenwagen Masters
    300031,     // Rosmalen Grass Court Championships
    57733494,   // badminton event
    124291191,  // Arab Badminton Championships
    799591,     // World Deaf Badminton Championships
    799450,     // World Junior Badminton Championships
    799434,     // World Senior Badminton Championships
    1270143,    // Danish Junior Badminton Championships
    10876319,   // Taiwanese Team Badminton Championships
    257285,     // badminton at the Pan Arab Games
    13507423,   // Badminton at the Commonwealth Youth Games
    799611,     // Badminton Europe
    799258,     // Chinese Badminton Association
    941150,     // Plusregio
    115208838,  // Gym Waasland
    2283121,    // Omloop van het Waasland
    3406166,    // Pro League
    383128,     // LEN
    223313,     // BRIT Awards
    24616,      // Empire Awards
    655089,     // International Indian Film Academy Awards
    384139,     // Africa Movie Academy Awards
    843417,     // British Open Squash Championships
    112610633,  // Hypotheticon
    27889498,   // boxing match
    246747,     // World Amateur Boxing Championships
    2646753,    // Asian Amateur Boxing Championships
    148319,     // planetarium
    1439650,    // Fotogramas de Plata
    6128710,    // Merano Cup
    2088357,    // musical ensemble
    7315155,    // research center
    4905959,    // Big League World Series
    2377567,    // Senior League World Series
    6313327,    // Junior League World Series
    273039,     // World Youth Day
    11570731,   // special discount ticket
    1001620,    // Cook Out Clash at Bowman Gray Stadium
    498162,     // census-designated place in the United States
    986756,     // census in Japan
    4350735,    // census in Brazil
    4350754,    // census in Canada
    5058977,    // Census in Poland
    3518943,    // census in Turkey
    48790556,   // census in Colombia
    124166774,  // census in Honduras
    5058971,    // census in Australia
    19799894,   // census in Argentina
    111979285,  // Lithuanian census
    1345528,    // United States census
    126925491,  // New Jersey state censuses
    3405306,    // population census in Hong Kong
    2062773,    // Evrovizijska Melodija
    61355526,   // Sčítanie obyvateľov, domov a bytov
    107862888,  // Volleks- a Wunnengszielung
    5696789,    // Antena de Oro
    108738758,  // Assen 500
    13745128,   // Koninklijke Nederlandsche Schaatsenrijders Bond
    29300714,   // international association
    47000586,   // Nations Cup Association
    58483083,   // dramatico-musical work
    15116915,   // show
    627933,     // dog show
    24634210,   // podcast show
    12538685,   // roadshow
    1129867,    // NAMM Show
    132171575,  // The Kenny Kerr Show
    909430,     // GMA Dove Award
    27927857,   // baseball game
    13027888,   // baseball team
    6631808,    // baseball league
    595452,     // baseball venue
    1184076,    // Baseball Federation of Asia
    5360559,    // Japan High School Baseball Federation
    124749680,  // Caribbean Professional Baseball Confederation
    1069698,    // Major League Baseball All-Star Game
    12613663,   // Nippon Professional Baseball draft
    2922711,    // bowl game
    2142464,    // World Bowl
    46135307,   // nation at sport competition
    1150939,    // MTV Asia Awards
    2005425,    // Sultan Azlan Shah Cup
    15928042,   // TVB Star Awards Malaysia
    11424,      // film
    24856,      // film series
    2404861,    // Oceania Cup
    1210183,    // SANZAAR
    36103,      // Royal Rumble
    2955194,    // USA Outdoor Track and Field Championships
    795848,     // BET Awards
    48635101,   // Jewish delicatessen
    31920,      // swimming
    6157864,    // Japan Swimming Federation
    1205300,    // German Swimming Federation
    606671,     // FINA World Junior Swimming Championships
    357380,     // indoor swimming pool
    17232649,   // news website
    112671154,  // Table Hockey European Championships
    29144,      // National Hockey League All-Star Game
    8030925,    // Women's Hockey Junior World Cup
    482998,     // Asia League Ice Hockey
    16716711,   // Men's EuroHockey Nations Championship
    11782483,   // Women's EuroHockey Nations Championship
    1429205,    // German floorball association
    1361966,    // First League of the Federation of Bosnia and Herzegovina
    12620556,   // Korea Drama Awards
    483271,     // MAMA Awards
    2878855,    // bus rapid transit
    2122052,    // qualification event
    614735,     // Federação de Futebol do Estado do Rio de Janeiro
    981133,     // Liga Portuguesa de Futebol Profissional
    3394495,    // Mestre Mateo Awards
    20012074,   // Lega Pro
    1092612,    // Lega Nazionale Professionisti
    2427920,    // Lega Serie A
    15804,      // Serie A
    677397,     // Serie C1
    130367385,  // European Rubik's Cube Championship
    2925703,    // British Academy Television Craft Awards
    4484477,    // FA Cup Final
    12352201,   // Malferma Tago de UEA
    1194340,    // UCI Europe Tour
    1039648,    // UCI Oceania Tour
    71580493,   // UCI ProSeries
    1063430,    // UCI America Tour
    3298301,    // Match des Champions
    483242,     // laboratory
    112610344,  // Cytricon
    115776212,  // Hop Gala edition
    517763,     // Maurice Revello Tournament
    2739000,    // Intercontinental Cup
    1269315,    // EuroGames
    108750339,  // Unión de Actores Awards
    1137234,    // Country Music Association Awards
    106805967,  // performed music
    96206648,   // Indiana Film Journalists Association Awards
    40244,      // marathon
    578794,     // London Marathon
    679106,     // Madrid Marathon
    2629108,    // Antwerp Marathon
    3479346,    // wushu
    300920,     // esports
    48004378,   // esport competition
    1008072,    // Home Run Derby
    2298921,    // photo shoot
    7187943,    // photowalking
    11992175,   // Norwegian Skiing Championships
    1162779,    // FIS Junior World Nordic Skiing Championships
    11303,      // skyscraper
    127867755,  // LOVB Pro
    11700336,   // Silver Condor Awards
    1194023,    // Yokohama DeNA BayStars
    2166013,    // wiki hosting service
    108802973,  // Wiki Loves X campaign
    28521056,   // Wiki Indaba
    81881796,   // Wiki Exploration Program
    61791944,   // Wikidata Lab
    129166943,  // Wikidata workshop
    120437489,  // Wikitown project
    17633526,   // Wikinews article
    47647413,   // Wikipedia:WikiGap
    79786,      // Wikipedia:Meetup
    24231964,   // Wikipedia:Meetup/Malaysia
    65128942,   // Wikipedia BUNGAKU
    17194091,   // Wikipedia town
    79695687,   // Wikipedia Day unconference
    64800812,   // Wikipedianische KulTour
    4167836,    // Wikimedia category
    3568028,    // Wikimedia movement
    22337472,   // Wikimedia user group
    14827288,   // Wikimedia project
    14204246,   // Wikimedia project page
    13406463,   // Wikimedia list article
    19692233,   // Wikimedia list of persons
    98645843,   // Wikimedia music-related list
    21484471,   // Wikimedia article covering multiple topics
    19688772,   // Wikimedia regional meeting place
    18694077,   // Wikimedia CEE Meeting
    54913642,   // Wikimedia Polska conference
    56035565,   // Wikimedia community
    10876435,   // Wikimedia Taiwan
    13098516,   // Wikimedia Indonesia
    15735556,   // Wikimedia Czech Republic
    26267937,   // Algeria Wikimedians
    71954356,   // WikiGap
    106192711,  // wikicontest
    16695773,   // WikiProject
    113990878,  // WikiSendero Andes
    483279,     // Wikimania
    54856428,   // WikiCite conference
    54856399,   // WikiCon
    15231127,   // WikiCon
    85443050,   // WikiCon Portugal
    3242178,    // WikiConference India
    131395419,  // WikiConference Kerala
    37942728,   // Ukrainian Wikiconference
    42406391,   // Semantic MediaWiki Conference
    42305379,   // French-speaking WikiConvention
    108390245,  // North-West Russia Wiki-Historians User Group events
    4654165,    // Category:Wikipedia meetups
    115270134,  // EMLex Wiktionary HackFest
    123989453,  // WBSC Asia
    64485080,   // Pégase
    10387949,   // Winter Universiade
    1375427,    // European championship
    3535276,    // Six Nations Under 20s Championship
    950844,     // Women's Super League
    1955280,    // music competition
    134350581,  // Frederiksberg Criterium
    1153257,    // NAACP Image Awards
    10871950,   // National Para Games of the People's Republic of China
    16249975,   // homage
    124801455,  // Écrans mixtes 2024
    360418,     // construction site
    315463,     // Dresdner Eislöwen
    74124757,   // European Masters Ahletics Championships – Indoor
    111682272,  // Satellite
    15728786,   // Feroz Awards
    372681,     // Amaury Sport Organisation
    617378,     // Worldcon
    846298,     // Japan Racing Association
    8036525,    // World Wonder Ring Stardom
    1193068,    // izakaya
    108457724,  // Ambyar Awards
    16022392,   // edit-a-thon
    117391269,  // Lamego, Cidade Poema
    212828,     // Goya Awards
    3045646,    // Mostra de València
    3327327,    // Club Natació Sant Andreu
    112574007,  // Sonor Awards
    1999974,    // Asociación de Clubs de Baloncesto
    200764,     // bookstore
    20655841,   // Super League Vrouwenvoetbal
    124312375,  // Sculptua edition
    7824594,    // Top Rank
    5798,       // Top 14
    1376281,    // Europe Top-16
    1786678,    // Matchroom Sport
    459975,     // Jigoro Kano Cup
    636146,     // All Japan Sports Prototype Championship
    106697764,  // All Japan Road Race Championship Sugo Round
    106712966,  // All Japan Road Race Championship Motegi Round
    106712980,  // All Japan Road Race Championship MFJ GP Round
    106737001,  // All Japan Road Race Championship Suzuka Round
    106712954,  // All Japan Road Race Championship Okayama Round
    106725499,  // All Japan Road Race Championship Tsukuba Round
    106705839,  // All Japan Road Race Championship Autopolis Round
    106763118,  // All Japan Road Race Championship Motegi 2&4 Round
    106808186,  // All Japan Road Race Championship Suzuka 2&4 Round
    106763105,  // All Japan Road Race Championship Autopolis 2&4 Round
    107361639,  // Asia Road Race Championship Chang Round
    107388035,  // Asia Road Race Championship Buddh Round
    107376452,  // Asia Road Race Championship Johor Round
    107361811,  // Asia Road Race Championship Sentul Round
    107361737,  // Asia Road Race Championship Suzuka Round
    107365627,  // Asia Road Race Championship Zhuhai Round
    107365603,  // Asia Road Race Championship Sepang Round
    107361731,  // Asia Road Race Championship The Bend Round
    2912397,    // single-day road race
    201971,     // water polo at the World Aquatics Championships
    5016107,    // Miss Universe Vietnam
    219586,     // A-League Men
    111936350,  // Pacific Four Series
    1325952,    // Lou DiBella
    188509,     // suburb
    391006,     // Premier League of Bosnia and Herzegovina
    74570489,   // art auction
    16743915,   // Met Gala
    57204392,   // W Series
    855544,     // Biathlon Junior World Championships
    577698,     // MLS Cup
    4044501,    // Monterey Sports Car Championships
    3270632,    // national championship
    101084513,  // Produ Awards
    1139408,    // Liga FPD
    4534009,    // South American Beach Games
    839515,     // División Mayor del Fútbol Profesional Colombiano
    2419773,    // European Fencing Championships
    2467765,    // European Fencing Confederation
    19577496,   // Pan American Fencing Championships
    54114,      // boulevard
    947114,     // Academy of Motion Picture Arts and Sciences of Argentina Awards
    123556202,  // ProCar 4000
    1199476,    // Panam Sports
    110738365,  // Joy Awards
    110757450,  // Joy Awards 2022
    110757460,  // Joy Awards ceremony
    3117505,    // Gaudí Awards
    6101891,    // Reial Federació Espanyola de Patinatge
    36921,      // HC Davos
    1839475,    // Bobsleigh European Championship
    1785497,    // Vrouwen Eredivisie
    202699,     // Swiss Super League
    694394,     // 2. Liga Interregional
    2907539,    // Board of Control for Cricket in India
    3534210,    // UNAF Women's Tournament
    2519576,    // Real Federación Española de Atletismo
    99430267,   // FIM CEV Moto3 Junior World Championship Le Mans Race
    99520829,   // FIM CEV Moto3 Junior World Championship Estoril Race
    99430206,   // FIM CEV Moto3 Junior World Championship Portimão Race
    99430607,   // FIM CEV Moto3 Junior World Championship Motorland Aragón Race
    99430287,   // FIM CEV Moto3 Junior World Championship Barcelona-Catalunya Race
    99431720,   // FIM CEV Moto3 Junior World Championship Circuito de Jerez Race
    99430809,   // FIM CEV Moto3 Junior World Championship Circuito de Navarra Race
    99430799,   // FIM CEV Moto3 Junior World Championship Circuito de Albacete Race
    99432320,   // FIM CEV Moto3 Junior World Championship Circuit Ricardo Tormo Race
    105832563,  // BizBarcelona
    1077012,    // annual general meeting
    116775032,  // Saint-Phil en Fête
    896903,     // round table
    125171624,  // Park World Excellence Awards
    60647261,   // Luxembourgian Union of Skating
    1376684,    // European Hockey Federation
    16992788,   // European BMX Championships
    1247728, // European Triathlon Union
    4154060, // Dyke March
    1548436, // European Short Track Speed Skating Championships
    123705, // neighborhood
    928830, // metro station
    43638585, // Stolperschwelle
    314003, // Stolpersteine
    113145992, // Rehabilitation
    116258918, // Czech Ski Orienteering Championships
    2856528, // Czech cyclo-cross championships
    132348787, // Czech Open
    113680960, // Festival Animánie
    10870555, // report
    45897039, // Czech orienteering championships
    2867672, // European Canoe Association
    1770751, // UniCredit Czech Open
    4147738,    // Grand Prix of Mosport
    30094256,   // Grand Prix Winterthur
    11227712,   // KEIRIN Grand Prix
    107080092,  // Norsk Melodi Grand Prix entry
    99347157,   // Japanese motorcycle Grand Prix Asia Talent Cup Race
    86731227,   // Walk Bell John Awards
    99662712,   // J-GP2 Sugo Race
    99662602,   // J-GP2 Tsukuba Race
    99663085,   // J-GP2 Okayama Race
    99664737,   // J-GP2 MFJ Grand Prix
    99662853,   // J-GP2 Autopolis Race
    99540868,   // J-GP3 Sugo Race
    99540822,   // J-GP3 Motegi Race
    99540908,   // J-GP3 Okayama Race
    99540178,   // J-GP3 Autopolis Race
    99540929,   // J-GP3 MFJ Grand Prix
    99540889,   // J-GP3 Tsukuba Race
    100165825,  // JSB1000 Sugo Race
    100165758,  // JSB1000 Suzuka Race
    100165583,  // JSB1000 Motegi Race
    100166026,  // JSB1000 Okayama Race
    100165793,  // JSB1000 Autopolis Race
    99844297,   // ST600 Sugo Race
    99846964,   // ST600 Suzuka Race
    99843977,   // ST600 Motegi Race
    99844568,   // ST600 Tsukuba Race
    99844758,   // ST600 Okayama Race
    167824,     // Canada Games
    171408,     // Grey Cup
    55074683,   // Campeones Cup
    603773,     // lecture
    2300462,    // Streamy Awards
    19647972,   // Transgender Erotica Awards
    100255648,  // Pornhub Award
    3046190,    // ESPY Award
    4504495,    // award ceremony
    115915866,  // award ceremony edition
    500834,     // tournament
    67421136,   // presidential forum
    113813711,  // coin type
    106093190,  // 2 Euro commemorative coins
    47433,      // banknote
    28794013,   // design of a banknote or a coin
    8142,       // currency
    1570153,    // local currency
    28783456,   // obsolete currency
    526877,     // web series
    3405554,    // Prix de la Ville du Mont Pujols
    116689628,  // Grand Prix de France Féminin
    438311,     // coverage of Google Street View
    2279488,    // concessions in the Dutch public transport
    15238777,   // legislative term
    1812889,    // legislative session
    10655253,   // legislative session of the Riksdag
    847301,     // political campaign
    11642595,   // election campaign
    60589804,   // presidential campaign
    120734597,  // gubernatorial campaign
    20639925,   // campaign for the 2016 United States presidential election
    116884555,  // campaign for the 2024 United States presidential election
    192611,     // electoral unit
    355844,     // electoral list
    19571328,   // electoral result
    935741,     // election commission
    40231,      // public election
    15280243,   // mayoral election
    15261477,   // gubernatorial election
    4128673,    // gubernatorial elections in Russia
    75951701,   // regional election
    10553987,   // regional elections in Sweden
    3587397,    // French regional election
    48782035,   // Murcian regional election
    152450,     // municipal election
    3722112,    // municipal election in Italy
    10271631,   // municipal election in Brazil
    12322522,   // municipal election in Denmark
    13102763,   // municipal election in Luxembourg
    6936320,    // Municipal elections in Canada
    15154951,   // municipal elections in Finland
    3423767,    // Belgian municipal elections
    669262,     // primary election
    25080094,   // primary election in the United States
    11931634,   // Czech Senate election
    3587541,    // French senate elections
    24045394,   // Australian Senate election
    28333164,   // Philippine Senate election
    24333627,   // United States Senate election
    24397514,   // United States House of Representatives election
    11918804,   // eleccions regionals de Trentino-Tirol del Sud
    4861943,    // Barnsley Metropolitan Borough Council election
    102124796,  // regional council elections in Denmark
    28453813,   // Gwynedd Council election
    49637768,   // Taiwan county or provincial city council election
    15273379,   // Hong Kong Legislative Council election
    11267647,   // Japanese House of Councillors election
    2618461,    // legislative election
    6518201,    // legislative assembly election in India
    24410897,   // Legislative Yuan election
    25546358,   // Bermudian legislative election
    25455090,   // Macanese legislative election
    5518656,    // South Korean legislative elections
    25467156,   // Guamanian legislative election
    10712098,   // Church of Sweden elections
    15966540,   // local election
    60009059,   // local elections in Spain
    6664348,    // local elections in Poland
    5894489,    // Hong Kong local election
    7061379,    // Norwegian local elections
    10271409,   // Portuguese local elections
    17496410,   // United Kingdom local elections
    1376777,    // election in Japan
    5354906,    // election in Taiwan
    5354870,    // election in Vermont
    107394355,  // state election
    131206966,  // state election in Hesse
    131206971,  // state election in Bremen
    1292159,    // state election in Schleswig-Holstein
    1804250,    // state election in Lower Saxony
    1845840,    // state election in Bavaria
    25289192,   // state election in Hamburg
    896995,     // state election in Berlin
    1804251,    // state election in North Rhine-Westphalia
    1768624,    // state election in Saarland
    1804249,    // state election in Mecklenburg-Vorpommern
    1804253,    // state election in Saxony-Anhalt
    56185179,   // South Australian state election
    1057954,    // by-election
    7864918,    // UK Parliamentary by-election
    30572165,   // Scottish Parliament by-election
    116131032,  // Senedd by-election
    52362967,   // Quebec provincial by-election
    51879476,   // South Australian House of Assembly by-election
    6508670,    // leadership election
    2539315,    // Liberal Democratic Party (Japan) leadership election
    5255736,    // Democratic Party presidential primaries
    3586951,    // Quebec general election
    3586928,    // Alberta general election
    3586948,    // Ontario general election
    3586944,    // Manitoba general election
    3586952,    // Saskatchewan general election
    3586936,    // British Columbia general election
    120668510,  // Saami parliamentary election
    10271596,   // parliamentary election in Madeira
    28649370,   // Philippine House election
    14516417,   // election to Chamber of Deputies of the Czech parliament
    4289143,    // Provincial elections in the Netherlands
    2977046,    // Belgian provincial election
    47466753,   // Belgian district elections
    5172025,    // Cornwall Council election
    117249541,  // Somerset Council election
    1006573,    // Swiss Federal Council election
    7559840,    // Somerset County Council election
    109682730,  // Indonesian People's Representative Council election
    24039278,   // Council of the Isles of Scilly election
    1128324,    // European Parliament election
    126456824,  // European Parliament election in France
    126456703,  // European Parliament election in Germany
    9378920,    // European Parliament election in Poland
    11890047,   // French departmental election
    125422885,  // Speaker of the United States House of Representatives election
    26466721,   // special election to the United States House of Representatives
    14006248,   // elections in France
    8774346,    // elections in Andalusia
    11918772,   // Elections to Galician parliament
    4164871,    // position
    599151,     // official
    294414,     // public office
    35535,      // police force
    171341,     // almanac
    66436502,   // race track layout
    1593564,    // army aviation component
    811704,     // rolling stock class
    3511132,    // series
    108028209,  // designation type
    1002697,    // periodical
    2015628,    // open space
    167270,     // trademark
    690109,     // branch of service
    105596363,  // national submarine fleet
    697175,     // launch vehicle
    277759,     // book series
    56019,      // military rank
    120451300,  // integrated circuit series
    1210109,    // hybrid integrated circuit
    123991206,  // discontinued legal form
    157031,     // foundation
    8102,       // slang
    220898,     // original video animation
    192350,     // ministry
    3143387,    // ministry of the Netherlands
    106719627,  // ministry of the Soviet Union
    15711797,   // finance ministry
    2155186,    // program block
    155076,     // juridical person
    3516239,    // growth rate
    3010369,    // opening ceremony
    13454063,   // closing ceremony
    130192067,  // Olympic Games opening ceremony
    94998068,   // Olympic Games closing ceremony
    3178415,    // Summer Youth Olympic Games
    3643463,    // Olympic cauldron
    9352238,    // Olympic torch relay
    938381,     // bid for the Olympic Games
    1915979,    // medal table
    3890208,    // Olympic medal table
    52506277,   // Paralympic medal table
    65075564,   // Men's African Olympic Qualifier
    65075567,   // Women's African Olympic Qualifier
    18536594,   // Olympic sporting event
    26132862,   // Olympic sports discipline event
    844293,     // Olympic Council of Asia
    26213387,   // Olympic delegation
    84491920,   // Youth Olympic sporting event 
    3071046,    // Union of Arab National Olympic Committees
    1134131,    // European Youth Olympic Festival
    865588,     // committee
    778966,     // International Committee of the Mediterranean Games
    50393057,   // Paralympic sporting event
    46195901,   // Paralympics delegation
    27971968,   // constituency of the House of Commons
    110415260,  // unitary authority area
    112207981,  // Faircon
    1004,       // comics
    1760610,    // comic book
    14406742,   // comic book series
    867242,     // comics anthology
    213369,     // webcomic
    8343,       // clarinet
    8345,       // bass clarinet
    18534,      // metaphor
    2658935,    // beauty contest
    62391930,   // beauty pageant edition
    58863414,   // female beauty pageant
    193468,     // van
    2137778,    // government formation
    2178284,    // Belgian government formation
    2601792,    // Dutch cabinet formation
    19803532, // Koninginnedag
    5157911, // ConFusion
    1079023, // championship
    30304302, // National Park System unit
    2178268, // strip programming
    15097084, // heritage register
    16858238, // train and rail category
    1311, // winter
    15104915,   // mineral deposit
    2162458,    // raw material deposit
    5962346, // classification scheme
    691760, // Mediterranean Grand Prix
    3686319, // concorso a premi
    14659,      // coat of arms
    645466,     // national coat of arms
    473708, // home computer
    188589, // tokamak
    131409607, // Kommunalwahl in Hessen
    643918, // Bundesgartenschau
    1685451, // student society
    4511568, // Speedway World Pairs Championship
    131437023, // tax status
    53490595, // Guldbagge Awards ceremony
    46970,      // airline
    47988,      // identity document
    113255208,  // spacecraft series
    554211,     // State of the Union address
    5,          // human
    12695722,   // walking tour
    7725634,    // literary work
    1020008,    // market index
    223371,     // stock market index
    47465437,   // player season
    46999879,   // La Scala season
    1539532,    // sports season of a sports club
    53642685,   // Canadian football team season
    1301371, // computer network
    7241458, // presidential proclamation
    5021202, // California ballot proposition
    1400264, // artist collective
    10541491, // legal form
    2243695, // Eerste Kamerverkiezing
    1643989, // legal tender
    11394606, // Internal Subdivision
    5640597, // Hakone Ekiden
    3917681, // embassy
    15284634, // executive term
    7278, // political party
    414147, // academy of sciences
    55725952, // tractor model
    23034867, // bus chassis model
    108059851, // natural monument
    117048953, // experimental catchment observatory
    1092864, // Pau Grand Prix
    35127, // website
    50331459, // S-Bahn route
    1639207, // nature restoration
    3331189, // version, edition or translation
    2672742,    // vote counting
    97124318,   // universal vote by mail
    59284,      // SummerSlam
    35516,      // WrestleMania
    41263,      // budget
    271855,     // government budget
    7887966,    // United Kingdom budget
    5030766,    // Canadian federal budget
    4825004,    // Australian federal budget
    210167,     // video game developer
    278329,     // WWE Armageddon
    548126,     // anime convention
    60593, // VLS-1
    1028645, // dead mall
    15221623, // bilateral relation
    41487,      // court
    190752,     // supreme court
    19953632, // former administrative territorial entity
    475050, // federal district
    1259759, // miniseries
    27796293, // United States vice-presidential debate
    614084, // consumer cooperative
    1935728, // stone circle
    8502, // mountain
    49958, // opinion poll
    34493, // motorcycle
    10929058, // product model
    1663817, // European Inline Speed Skating Championships
    116372166, // Eastern Orthodox archdiocese
    2920759, // bus terminus
    15275719, // recurring event
    59759, // Survivor Series
    1044551, // WWE Backlash
    28933155, // news aggregation website
    7406919,    // service
    166142, // application
    53062420, // nation in the Junior Eurovision Song Contest
    1445650,    // holiday
    277436,     // bank holiday
    1197685,    // public holiday
    5257307, // prize
    26887428, // nation at competition
    16974973, // Erondegemse Pijl
    674950, // residential area
    124399988, // Swedish state inquiry entity
    16543246, // literary competition
    30587581, // private member's bill
    95612266, // World Soundtrack Awards ceremony
    5386,   // auto racing
    877998, // students' union
    57036, // sauna
    26279994, // nation at the Mediterranean Games
    540724, // T in the Park
    524572, // term of office
    68295960, // Swedish government agency
    64272108, // cultural festival
    126682969, // new media arts festival
    24335959, // trolleybus line
    46122817, // basic road
    2993180, // French Socialist Party congress
    36378899, // parliamentary group of the French National Assembly
    4686866, // advisory board
    196756, // certificate
    27968043, // festival occurrence
    58687420, // cultural event
    1412219, // log flume ride
    178790, // labor union
    897119, // German Bowl
    446354, // German Protestant Church Assembly
    1153670, // DIN-Norm
    168210, // Internet forum
    97768050,   // Formula 2 race
    11924610,   // Formula One Grand Prix
    836943,     // German Formula Three Championship
    23622,      // dictionary
    3327521,    // online dictionary
    1444571, // franking
    375471, // student exchange program
    61014588, // Zlot Zimowy
    23700466,   // session
    17276616,   // IOC Session
    13147896,   // plenary session
    1875630, // aircraft fleet
    1141470, // double act
    194166, // consortium
    105674, // running
    3498533, // natural gas-fired power station
    127933,     // firework
    2699757,    // fireworks festival
    11081619,   // land
    7397,       // software
    11774490,   // broker-dealer
    1859105,    // World Police and Fire Games
    30634609, // heritage designation
    1198521, // night bus service
    2818080, // 6 Hours of Fuji
    43501, // zoo
    16913666,   // Academy Awards ceremony
    19311591,   // Golden Globe Award ceremony
    24569309,   // Tony Awards ceremony
    100796287,  // Golden Bell Awards ceremony
    22098766,   // Golden Horse Awards Ceremony
    27308988,   // César Awards ceremony
    27308875,   // Saturn Awards ceremony
    28057489,   // Hong Kong Film Awards ceremony
    1449648, // tabloid
    21089304, // honours list
    373899, // record chart
    15233622, // Perlis Open
    1937663, // commemorative medal
    1213562, // adaptation
    30849,      // blog
    14759031,   // travel blog
    8207109, // housing association
    7604686, // UK Statutory Instrument
    182129, // metropolitan area network
    105543609, // musical work/composition
    11016, // technology
    843564, // FIA WTCC Race of France
    1187667, // 3FM Serious Request
    1434748, // Flèche du Sud
    673220, // Top Spin
    15057021, // engine model
    1254933, // astronomical observatory
    113046319, // legislative period of the Swiss Federal Assembly
    61353107, // chronological summary
    6038651, // installation of the Yang di-Pertuan Agong
    378806, // Bluegreen Vacations Duel
    1077857,    // persona
    5602916,    // Green National Convention
    1185865,    // Democratic National Convention
    361909,     // Republican National Convention
    6541336,    // Libertarian National Convention
    751892, // caucus
    48204, // voluntary association
    186117, // timeline
    208322, // carousel
    100769007, // Seiyu Awards ceremony
    18208034, // indicator
    689044, // voucher
    4654196,    // ASEAN Summit
    20389574,   // Arab League summit
    467938,     // Summits of the Americas
    3578129, // ENP
    171448, // level crossing
    622812,     // talk show
    106942341,  // talk show episode
    565744, // Landeskirche
    35140, // performance
    1429577, // Floriade
    108696, // horse stud farm
    842537, // flower carpet
    2190251, // arts center
    50329691, // Winter X Games Europe
    848197,     // parliamentary group
    112064134,  // european days of knot tyers
    657069,     // historical reenactment
    986065,     // subdistrict
    2154027,    // motorway ring road
    18673040,   // Hong Kong Ordinance
    109645866,  // hobo tour
    24887239,   // Internet broadcast
    11664270,   // music program
    561068,     // specialty channel
    17558136,   // YouTube channel
    105416259,  // team of content creators
    90285444,   // International Council on Archives Congress
    2638480,    // church congregation
    5124642,    // civil decoration
    35232,      // Rio Carnival
    47223, // birthday
    653038, // Foire Internationale d'Art Contemporain
    184937, // Bilderberg Group
    2717948, // Oxegen
    112080326, // Bristol-Con
    604109, // Dansk Melodi Grand Prix
    27043990, // LEGO theme
    5464854, // Folsom Europe
    1628661,    // World Masters Games
    1376770,    // European Masters Games
    106616204,  // Pan-American Masters Games
    106909852,  // Winter World Masters Games
    155271, // think tank
    43325366, // viquitrobada
    16157186, // Castell Day of Saint Felix
    28640, // profession
    61974339, // esports team
    1161355, // fundraising
    2123557, // quiz show
    220499,     // streaming media
    63241860,   // streaming service
    59152282,   // video streaming service
    15590336,   // music streaming service
    43060,      // online music store
    28415750,   // APEC Economic Leaders Meeting
    10846717,   // COSCUP
    99383433,   // Pan Delta Super Racing Festival
    104921473, // presidential transition
    1580082, // small satellite
    26540, // artificial satellite
    979949, // International Eucharistic Congress
    39233713, // currency swap agreement
    123158862, // Gold Coast International Marine Expo
    123159322, // BMW GS Safari-Enduro
    4642518, // 4 Hours of Shanghai
    1829324, // architecture award
    230788, // grant
    11313106, // SPORTSEVENT
    278485, // hashtag
    26895936, // American football team season
    2389651, // children's music
    133215, // casino
    28854537, // San Diego Film Award ceremony
    15812736, // Grand Prix of Long Beach
    45308090, // Breakthrough Prize Ceremony
    80593337, // cancelled convention
    2393315, // dance festival
    1653073, // Christmas lights
    2992900, // Conference of the parties of the Convention on Biological Diversity
    1484397, // talent show
    657449,     // parade
    5495352,    // victory parade
    22947792,   // Moscow Victory Day Parade
    65963192,   // Folk festival parade
    7561653, // songwriting competition
    16997177, // Magic Weekend
    59108305, // World Cancer Congress
    130617, // tunnel boring machine
    15973082, // bagadoù national championship
    27030777, // State of the Map France
    19758733, // departmental council
    1408683, // International Festival of Geography
    849891, // casting
    3052748, // Korrika
    2100278, // panel discussion
    65676181, // Instagram account
    10688145, // season
    10980,      // triathlon
    43767888,   // triathlon competition
    3129162, // regional weekly
    1048801, // Paris Motor Show
    3470165, // Paris International Agricultural Show
    74595662, // Paris Peace Forum
    565507, // Munich Security Conference
    56378186, // Austrian Librarians Day Conference
    65962109, // HVDC submarine cable
    333016, // King's Day
    2298412, // solar power station
    6630631, // Christopher Street Day
    446643,     // job fair
    998672,     // book fair
    57305,      // trade fair
    673514,     // Renaissance fair
    18560568,   // periodic trade fair
    107474077,  // book fair edition
    125457035,  // trade fair edition
    123466619,  // science fair edition
    99936871,   // Thailand motorcycle Grand Prix Asia Talent Cup
    99347565,   // Malaysian motorcycle Grand Prix Asia Talent Cup Race
    99441974,   // Red Bull MotoGP Rookies Cup German motorcycle Grand Prix Race
    99443817,   // Red Bull MotoGP Rookies Cup Italian motorcycle Grand Prix Race
    99472045,   // Red Bull MotoGP Rookies Cup Austrian motorcycle Grand Prix Race
    99688417,   // Red Bull MotoGP Rookies Cup San Marino and Rimini's Coast motorcycle Grand Prix Race
    64418432, // statistical investigation
    8513, // database
    87487749, // former hotel
    116052632, // Dracula Digital edition
    115915872, // TIFF edition
    40008090, // European Theater Season
    191067, // article
    114079647, // Supreme Court of Ghana decision
    1153108, // multiplex
    24097670, // voting round
    860611, // International Congress of Mathematicians
    26877490,   // award, award nominees, award recipients or award ceremony
    69502391,   // bus rapid transit route
    7122263,    // Pacific-12 Football Championship Game
    181348,     // viaduct
    61658758,   // Yale Day of Data
    28870211,   // WordCamp US
    57775861,   // Sweet Adelines International quartet competition
    57776091,   // Sweet Adelines International chorus competition
    57776190,   // Sweet Adelines International harmony classic competition
    75964471,   // Libre Application Summit
    30935481,   // 2.5D musical
    4257172,    // ice show
    109537377, // North Korean calendar year
    131257, // intellectual property
    11022655, // case law
    3070242, // gaming convention
    1344963, // world championship
    1422253, // royal charter
    1785271, // cycling team
    6664352, // municipal district of Ireland
    2955692, // World Telemarking Championships
    10567925, // literary sign
    6815577, // memorialization
    790016, // extracurricular learning place
    100332920, // forest walk
    75171163, // Campus du libre
    1543677, // web conference
    13634881, // passageway
    74531574, // Clean Air Zone
    179179, // interest rate
    249565, // greenway
    2116321, // rail trail
    1435951, // food festival
    604733, // presentation
    858485, // high-speed railway line
    5783189, // Federal Congress of the PSOE
    115230003, // Web Summit conference
    47345468, // tennis tournament edition
    600134, // course
    25447168, // architectural project
    2717396, // Belgium Ypres Westhoek Rally
    20744439, // photograph series
    27642463, // deutsche Organisation des Gesundheitswesens
    385337, // deanery
    10357784, // redesign
    108269159, // datathon
    641406, // terrace
    98792435, // cultural center
    2393314, // dance troupe
    4573533, // amateur boxing
    1413606, // World Rowing Championships
    953806, // bus stop
    110832904, // Vietnamese case law
    1142930, // CubeSat
    24050099, // auto race
    106731089, // Dunedin Writers & Readers Festival 2021
    796919, // resignation
    51081147, // bid for sporting event
    11783626, // athletics meeting
    104418497, // online event
    113455816, // subathon
    545861, // legal hearing
    106635331, // MAREXAER
    1156895, // nomination
    62033032, // Apple media event
    118498024, // boxing event
    7166195, // pep rally
    15243209, // historic district
    42336871, // crowdfunding platform
    1457237, // media event
    44613, // monastery
    111019485, // Concertina
    46855, // hackathon
    4228029, // colloquium
    1156329, // auto show
    21473229, // photo contest
    2198291, // Belgian Athletics Championships
    6508605, // leaders' debate
    61051482, // kiescollegeverkiezing
    2138247, // Regional-Express
    13930359, // folk festival
    37654, // market
    2295790, // film screening
    316693, // Venus Berlin
    1238731, // vigil
    1415084, // Oldtimerfestival
    2100614, // general assembly
    115863228, // IJF World Tour Austria
    129372, // European Capital of Culture
    117463007, // hybrid conference
    2593801, // Kurentovanje
    110628558, // day of remembrance
    924286, // transport network
    4845365, // Shrove Tuesday
    99688750, // DJ set
    116267711, // library conference
    16252817, // European Hitchgathering
    1934817, // road running
    272281, // news conference
    19724435, // Total Dictation
    110156968, // conference presentation
    4220917, // film award
    15056993, // aircraft family
    123155338, // US government website
    123467224, // governor term
    926191, // ice dance
    122468729, // Grand Départ
    3304140, // meetup
    697749, // DB Class 101
    217102, // restoration
    857351, // International Chopin Piano Competition
    110616917, // Melodifestivalen edition
    137161, // Sukma Games
    1614379, // Emperor's oak
    811534, // remarkable tree
    1187491, // WCW Uncensored
    131451965, // digital arts event
    131451965, // digital arts event
    837766, // local authority
    3244038, // public establishment of an administrative nature
    21850100, // municipal flag
    2613738, // computer expo
    123159952, // democracy and power study
    507619, // retail chain
    166788, // biathlon
    90298865, // drugstore chain
    1070962, // Lucca Comics & Games
    5154611, // community
    422695, // awareness day
    185220,     // LAN party
    5215299,    // dance party
    16034435,   // maritime party
    2458322,    // children's party
    3918409, // proposal
    1983921, // Bathurst 1000
    95074, // character
    60195382, // Sjezd
    476300,     // competition
    841654,     // competition
    23807345,   // competition
    76998317,   // sailing competition
    7225113, // political statement
    1837149, // flypast
    1523924,    // lime works
    10832530,   // ironworks
    1179791,    // glassworks
    7725310,    // series of creative works
    921773, // engineering activities and related technical consultancy
    151885, // concept
    1050739, // WWE Judgment Day
    35769, // WWE programs
    101007233, // film critics association
    868250, // WWE No Way Out
    230186, // Pan American Games
    11488716, // foot race
    2874349, // Brazilian Air Force One
    59309547, // poetry award
    545769, // district of Ghana
    17566318, // human rights award
    1774587, // hospital network
    11483816, // annual event
    18631230, // Stiftung
    2881199, // public body in Spain
    114834437, // conference
    108095628, // conference edition
    5827809, // Elecciones estatales de México
    504703, // seminar
    3834389, // Ido Conference
    422638, // action plan
    170321, // wetland
    29380, // sponsored top-level domain
    277753, // Universal Forum of Cultures
    28043022, // proposed rail infrastructure
    20669447, // public inquiry
    29471082, // legislative term of the Parliament of Bergen, Norway
    4375512, // honorary title of the Republic of Tatarstan
    2514025, // posyolok
    1200183, // Fiber To The Home
    444456, // hot air balloon festival
    2993180, // French Socialist Party congress
    5783186, // National Congress of the People's Party
    1684600, // news magazine
    633506, // Estonian Song Festival
    2287074, // Continuous-flow intersection
    30106829, // proposed road
    13626210, // Eurofest
    186165, // web portal
    3422036, // recognition
    28928544,   // fashion award
    6501349,    // parking lot
    2143825,    // hiking trail
    3777387,    // former district of the canton of Bern
    7979513,    // Weekly Address of the President of the United States
    4502119,    // art group
    10274693    // literary group
    132202154, // Aikido Seminar
    1470855, // pocket park
    21044675, // televised debate
    112727436, // United States Shadow Senator election in the District of Columbia
    34508, // video recording
    18105680, // Cirque du Soleil show
    2507750, // weather warning
    124414048, // lights festival
    1958056, // science fiction convention
    627421, // launched roller coaster
    116856480, // former faculty
    5350543, // faculty of economics
    62032401, // album chart
    3722110, // Italian administrative election
    3253281, // artificial pond
    91128020, // UNESCO celebrations Třebíč
    27670147, // former protected area
    182683, // biennale
    846662, // game show
    2815206, // El Gouna International
    1262712, // gas pipeline
    107659169, // Africa Movie Academy Awards ceremony
    642682, // safari park
    14346334, // web television
    202570, // Ferris wheel
    842314, // bus lane
    383141, // list of international conferences on Afghanistan
    20107484, // rally
    190928, // shipyard
    4182287, // search engine
    55043, // gymnasium
    92480215, // greenfield international airport
    94993988, // commercial traffic aerodrome
    47785761, // Family Boomerang
    4539, // cooperative
    478847, // Amt
    18918137, // ice hockey game
    3586840, // 2011 French cantonal elections
    132863156, // Northern Hemisphere flu season
    132863167, // Southern Hemisphere flu season
    3276992, // Rotary Club
    3070220, // comic book convention
    127954038, // commemorative meeting
    18043413, // supermarket chain
    182060, // office
    257913, // Kōhaku Uta Gassen
    1123481, // design award
    6722708, // Macau Open
    1160329, // International Biology Olympiad
    2716481, // storyline
    7906416, // Super3 series
    2082626, // magnetic stripe card
    2338524, // motorsport racing track
    28464958, // Las Vegas Film Critics Society Awards
    2916312, // United States presidential primary
    27741749, // group of cats
    22652, // green space
    131647, // medallion
    107494101, // engineering award
    261604, // oil-fired power station
    6558431, // coal-fired power station
    118496986, // sports match
    1228895, // discothèque
    1365560, // university of applied sciences
    12193401, // Scout camp
    126701, // regatta
    21507948, // former village
    22276119, // Serbian legislative election
    22160105, // Greek legislative election
    17085643, // Eurosong - A MAD Show
    47003909, // printery
    23902005, // literary festival
    63976454, // X account
    15089348, // Turkvision Song Contest
    18406306, // Women's sambo championship in Russia
    2895132, // Presidential Address to the Federal Assembly of Russia
    476204, // election day
    30751147, // Tour of Gippsland
    123111273, // Traditional Melbourne Toy Run
    16971168, // NRL Auckland Nines
    23442, // island
    16643332, // lunar impact
    123760928, // disc golf event
    58037425, // ESEAP Hub
    48834796, // Society for the History of Authorship, Reading and Publishing
    129361855, // 22nd Century Initiative
    20400701, // Arab Urban Development Institute
    131899940, // PLATFORMA
    12180502, // Amanate of Riyadh
    30258605, // Institute of History, Czech Academy of Sciences
    3542768, // University of Belgrade Faculty of Philology
    2985090, // combined statistical area
    31157300, // LODLAM Summit
    2856460, // Motor City Open
    1363997, // Destination X
    9252000, // exhibit
    40434727, // team rivalry in sports
    5533934, // GeoTLD
    42032, // country code top-level domain
    85942930, // artistic duo
    131750534, // Barking and East London Round Table charity fireworks
    7095994, // Open Data Institute
    108512960, // Design Gallery Milano
    1604562, // Landesausstellung
    11734477,   // memorial stone
    1006442,    // Party congresses of the Christlich Demokratische Union Deutschlands
    820524,     // mountains classification
    132241,     // festival
    27968043,   // festival occurrence
    1046763,    // matsuri
    745325,     // jamboree
    4618,       // Carnival
    7039630,    // Nippon Scout Jamboree
    462100,     // wine festival
    3010205,    // celebration
    2903810, // semimonthly
    3653808, // IFSC Climbing World Youth Championships
    866210, // Synod of Bishops
    27491613, // Pacific typhoon season
    1920219, // social issue
    18949010, // Anime Revolution
    200023, // swimming center
    27044174, // research congress
    210064, // lighting
    124429535, // 2015 Cars & Croissants
    209465, // university campus
    1398920, // General council
    1673112, // Ironman 70.3
    59814090, // Super Demain
    4872714, // Battle of Waterloo reenactment
    5317357, // Dutch National Track Championships
    2667204, // Nederlandse Vereniging van Journalisten
    10655255, // The opening of the session of the Swedish parliament
    15817877, // scheduled air service
    10445146, // Centre for business history
    116150022, // Мероприятия Молодежной ассоциации финно-угорских народов
    116102883, // Eastern Partnership Summit
    6980735, // church election
    27917249, // electronic toll collection system
    18406136, // Russian championship of combat sambo
    584529, // humanoid robot
    11074464, // pornography website
    336181, // variety show
    11578774, // broadcasting program
    1530704, // corporate headquarters
    15306266, // train ticket
    22119581, // student conference
    76212517, // café chain
    4806651, // Asian Games sports
    126896673, // district of South Korea (non-autonomous)
    462058, // student loan
    4348600, // shopping channel
    110172738, // CEFED
    1163166, // water slide
    294297, // public bookcase
    1740966, // public transport bus service
    113040113, // national inventory of intangible cultural heritage
    17078133, // residential care
    1427735, // Fleet Air Arm
    992060, // fun run
    540130, // Senate of Berlin
    52045923, // I Annotate
    15712205, // intercity bus company
    11859675, // Finnjamboree
    110055303, // rocket model
    45767416, // parliamentary term in Malawi
    45380910, // National Assembly term in Zambia
    25302965, // Nastri d'argento Awards ceremony
    988108, // club
    1149652, // district of India
    4907414, // bikini contest
    12371153, // Estonian Youth Song and Dance Celebration
    681875, // utility model
    11890619, // hiking area
    207137, // curriculum
    55089188, // European Research Council Advanced Grant project
    49833, // wind turbine
    219423, // mural
    28870409, // WordCamp Cologne
    17296543, // Klaĉ-Kunveno Post-Somera
    1067715, // unconference
    68691239, // Celtic Knot
    28314507, // recurring entity
    178651, // interview
    20784907, // motorcycle racing team
    108369092, // City region
    114364435, // performance piece
    569500, // community health center
    44665130, // proposed bridge
    62500932, // digital humanities project
    41795373, // redistribution market
    21094819, // parliamentary term in the United Kingdom
    47762931, // TEDxRennes
    3587148, // French legislative election
    2026833, // garden square
    47000326, // season premiere at La Scala
    740777, // merchandising
    113880369, // hacker convention
    130267212, // hacker camp
    500415, // BarCamp
    2588110, // aisled house
    1779930, // bypass
    1362233, // Upper Lusatian house
    2935206, // Camp for Climate Action
    1107656, // garden
    15633604, // Stammtisch
    1169870, // educational trail
    57583, // International Motor Show Germany
    2089242, // pool play
    61729985, // Little Italy
    122752731, // Association of Network Marketing Professionals
    2085515, // minutes
    251868, // Halloween
    60767305, // neobank
    18248981, // legislative election in Colombia
    31603, // draft
    110324066, // religious drama
    2259749, // subject olympiad
    130191918, // GLOBSEC Forum
    1421331, // open house
    117427452, // Dagstuhl Seminar
    66317493, // reception
    100707160, // Wikimedia Czech Republic competition
    27464969, // proposed tunnel
    27494565, // county elections
    1550193, // State Opening of Parliament
    355567, // noble title
    336416, // juggling convention
    176799, // military unit
    21707777, // box office
    19952562, // offshore racing
    113813216, // Zac en Scène
    19361199, // Ain Sud
    228388, // train ride
    2398990, // technology museum
    104411569, // Wikiexpedition
    11407181, // graduation ceremony
    843360, // Maccabiah Games
    515449, // Regionalbahn
    7408, // imperial examination
    56582861, // Route impériale
    56072658,   // electoral district in Indonesia
    130377399,  // electoral district of the Netherlands
    6591764,    // provincial electoral district of New Brunswick
    26654367, // former county
    249556, // railway company
    178692, // crane
    11446400, // 女子高等師範学校
    1622132, // galeas
    155511, // plant nursery
    110346117, // Porträttbok
    63981919, // sport rivalry
    112793351, // casa consistorial
    830528, // velodrome
    7209458, // Police Force
    955824, // learned society
    1462047,    // vilayet
    1420,       // car
    178193,     // steamboat
    655677,     // branch line
    917575, // Games of the Small States of Europe
    119517896, // BarCamp Yerevan
    314729, // state government in Germany
    2514663, // worship
    184973, // internet radio
    98381912, // online scholarly workshop
    7935096, // virtual event
    112611340, // election in a constituency
    123028916, // 日本維新の会代表選挙
    2317127, // Teacups
    110013395, // theatrical work
    5058981, // census in the United Kingdom
    3062261, // online exhibition
    108151198, // Imperial Procession by motorcar after the Ceremony of the Enthronement
    108151420, // Court Banquets after the Ceremony of the Enthronement
    4202178, // Ceremonies of the Accession to the Throne
    111909369, // Melbourne Cup Day
    2904016, // BioBlitz
    15900647, // convention series
    65152203, // 2019 Bulgarian local elections
    20521099, // Melodies of the Sea and Sun
    10977433, // protected area of Ukraine
    55771466, // Legendy
    339152, // Vienna New Year's Concert
    70629395, // Entretiens de Belley
    104761764, // government forum
    1923282, // World Cup of Pool
    105501529, // House of Commons Select Committee
    52253007, // Free Knowledge Advocacy Group EU meeting
    112040174, // Cymrucon
    1420027, // fishing port
    19563580, // rail yard
    5058978, // census in the Philippines
    227857, // sugar refinery
//     , // 
//     , // 
//     , // 
//     , // 
//     , // 
//     , // 
};

inline bool has_ignored_class(uint32_t c) noexcept
{
    return ignoredClasses.contains(c);
}

bool has_ignored_class(std::ranges::range auto const& r) noexcept
{
    for(auto&& c : r)
    {
        if(ignoredClasses.contains(c))
        {
            return true;
        }
    }

    return false;
}


inline const unstable_unordered_map<uint32_t, uint32_t> classRemap =
{
    // -> war
    {104212151, 198}, // series of wars
    {24336068 , 198}, // mythical war

    // -> war crime
    {494497, 135010}, // Nazi roundup
    {715434, 135010}, // German war crimes
    {701321, 135010}, // Japanese war crimes

    // -> civil war
    {1348385, 8465}, // war of succession
    {3119160, 8465}, // Colombian civil war

    // -> conflict
    {350604   , 180684}, // armed conflict
    {2912528  , 180684}, // trade conflict
    {2672648  , 180684}, // social conflict
    {115431196, 180684}, // violent conflict
    {108809873, 180684}, // political conflict
    {11422542 , 180684}, // international conflict
    {10528555 , 180684}, // police-civilian conflicts
    {5683226  , 180684}, // environmental conflict
    {369799   , 180684}, // low-intensity conflict
    {107637520, 180684}, // border conflict
    {42750320 , 180684}, // border incident
    {766875   , 180684}, // ethnic conflict
    {13634374 , 180684}, // ethnic violence
    {467033   , 180684}, // feud
    {79084310 , 180684}, // clash
    {650711   , 180684}, // combat
    {997267   , 180684}, // skirmish
    {7292715  , 180684}, // range war
    {7292715  , 180684}, // asymmetric warfare
    {1078001  , 180684}, // territorial dispute

    // -> attack
    {2001676, 1174599}, // offensive
    {5176896, 1174599}, // counter-offensive

    // -> battle
    {26913948, 178561}, // legendary battle
    {15047343, 178561}, // battle of the Paraguayan War
    {48767773, 178561}, // engagement
    {680838  , 178561}, // ambush

    // -> revolution
    {6107280, 10931}, // revolt
    {3828206, 10931}, // Matxinada
    {3119121, 10931}, // revolutionary war
    {29502  , 10931}, // proletarian revolution

    // -> rebellion
    {1155622 , 124734}, // slave rebellion
    {1323212 , 124734}, // insurgency
    {511866  , 124734}, // mutiny
    {8191556 , 124734}, // ikki
    {10931495, 124734}, // shizoku rebellion
    {138796  , 124734}, // resistance movement
    {21168880, 124734}, // resistance
    {1753200 , 124734}, // intifada

    // -> riot
    {6024855 , 124757}, // tax riot
    {5465517 , 124757}, // food riot
    {3588250 , 124757}, // ethnic riot
    {2920604 , 124757}, // prison riot
    {7579940 , 124757}, // sports riot
    {13427116, 124757}, // peasant revolt
    {686984  , 124757}, // civil disorder
    {11079230, 124757}, // anti-missionary riot in China
    {7897387 , 124757}, // unrest

    // -> massacre
    {750215   , 3199915}, // mass murder
    {473814   , 3199915}, // amok
    {7539194  , 3199915}, // slave raid
    {21175615 , 3199915}, // school massacre
    {177716   , 3199915}, // pogrom
    {124612203, 3199915}, // massacre of civilians

    // -> disease outbreak
    {18123741 , 3241045}, // infectious disease
    {44512    , 3241045}, // epidemic
    {959265   , 3241045}, // cholera outbreak
    {112193867, 3241045}, // class of disease
    {113242098, 3241045}, // monkeypox epidemic
    {89252833 , 3241045}, // chikungunya outbreak

    // -> historical country
    {187587   , 3024240}, // empire
    {22085013 , 3024240}, // eparchy
    {417175   , 3024240}, // kingdom
    {6256     , 3024240}, // country
    {182034   , 3024240}, // hegemony
    {3117863  , 3024240}, // isolated human group
    {99541706 , 3024240}, // historical unrecognized state
    {115857096, 3024240}, // aspect of history in a geographic region

    // -> historical period
    {17522177 , 11514315}, // age
    {238526   , 11514315}, // golden age
    {6428674  , 11514315}, // era
    {256408   , 11514315}, // era name
    {698350   , 11514315}, // Korean era name
    {845538   , 11514315}, // Chinese era name
    {24706    , 11514315}, // Japanese era name
    {4375074  , 11514315}, // calendar era
    {8432     , 11514315}, // civilization
    {815962   , 11514315}, // colonization
    {816829   , 11514315}, // periodization
    {186081   , 11514315}, // time interval
    {17524420 , 11514315}, // aspect of history
    {1620908  , 11514315}, // historical region
    {105038213, 11514315}, // societal transformation
    {15649510 , 11514315}, // satrapy of the Sasanian Empire

    // -> archaeological culture
    {11042    , 465299}, // culture
    {1792644  , 465299}, // art style
    {185087   , 465299}, // abjad
    {8192     , 465299}, // writing system
    {34770    , 465299}, // language
    {45762    , 465299}, // dead language
    {436240   , 465299}, // ancient language
    {38058796 , 465299}, // extinct language
    {2315359  , 465299}, // historical language
    {109551565, 465299}, // sub-set of literature

    // -> archaeological site
    {755017  , 839954}, // tell
    {381885  , 839954}, // tomb
    {35509   , 839954}, // cave
    {1787688 , 839954}, // Rondel enclosure
    {1156875 , 839954}, // undeciphered writing system
    {10855061, 839954}, // archaeological find
    {959782  , 839954}, // archaeological excavation
    {19757   , 839954}, // Roman theatre
    {56055312, 839954}, // sepulchral monument
    {15661340, 839954}, // ancient city
    {11558926, 839954}, // underwater archaeological site
    {2562511 , 839954}, // footprint
    {731966  , 839954}, // nymphaeum
    {88205   , 839954}, // castrum

    // -> lake
    {55965244, 23397}, // paleolake
    {47486890, 23397}, // former lake

    // -> volcanic eruption
    {747501   , 7692360}, // Plinian eruption
    {1067217  , 7692360}, // phreatic eruption
    {1639789  , 7692360}, // phreatomagmatic eruption
    {11387245 , 7692360}, // lateral eruption
    {115486763, 7692360}, // magmatic eruption
    {611880   , 7692360}, // Strombolian eruption

    // -> earthquake
    {7850171  , 7944}, // tsunami earthquake
    {5300157  , 7944}, // doublet earthquake
    {2737176  , 7944}, // intraplate earthquake
    {727990   , 7944}, // megathrust earthquake
    {17006934 , 7944}, // deep-focus earthquake
    {11639848 , 7944}, // multi-segment earthquake
    {7312186  , 7944}, // remotely triggered earthquake
    {18460157 , 7944}, // 超巨大地震
    {88023758 , 7944}, // 小笠原諸島西方沖地震
    {489523   , 7944}, // earthquake swarm
    {114041309, 7944}, // earthquake sequence
    {1772541  , 7944}, // earthquake in New Zealand
    {3510594  , 7944}, // earthquake in Japan
    {11615877 , 7944}, // Geiyo earthquake
    {24039748 , 7944}, // Kantō earthquakes
    {17392370 , 7944}, // Tōnankai earthquake
    {23828704 , 7944}, // Tokachi earthquake
    {11505155 , 7944}, // Hyūga-nada earthquake
    {30922173 , 7944}, // Izu Ōshima Kinkai earthquake
    {6963731  , 7944}, // Nankai megathrust earthquake
    {11496705 , 7944}, // off Bōsō earthquake
    {121832207, 7944}, // off Miyagi earthquake
    {7446977  , 7944}, // off Sanriku earthquake
    {11593211 , 7944}, // off Fukushima earthquake
    {121832475, 7944}, // off Nemuro Peninsula earthquake
    {23828916 , 7944}, // east off Chiba earthquake

    // -> treaty
    {625298 , 131569}, // peace treaty
    {9557810, 131569}, // bilateral treaty
    {6934728, 131569}, // multilateral treaty
    {864737 , 131569}, // Unequal Treaties
    {107706 , 131569}, // armistice
    {11122  , 131569}, // treaty of the European Union
    {814924 , 131569}, // Treaty of Accession to the EU
    {1691434, 131569}, // United Nations treaty
    {2006324, 131569}, // agreement
    {252550 , 131569}, // trade agreement
    {727002 , 131569}, // charter
    {93288  , 131569}, // contract
    {6944158, 131569}, // defense pact

    // -> disappearance
    {1288449  , 3030513}, // enforced disappearance
    {7884274  , 3030513}, // unexplained disappearance
    {110903080, 3030513}, // unexplained disappearance of a human
    {715450   , 3030513}, // North Korean abductions of Japanese citizens

    {1384277, 831663}, // military expedition -> military campaign
    {188494, 19902850}, // witch hunt -> witch trial
    {10551516, 111161}, // church council -> synod

    // -> military operation
    {15835236, 645883}, // military action
    {646740  , 645883}, // landing operation
    {582956  , 645883}, // punitive expedition
    {20082270, 645883}, // peacekeeping mission
    {3354903 , 645883}, // clandestine operation
    {1143267 , 645883}, // humanitarian intervention

    // -> military occupation
    {17768966, 188686}, // capture
    {217901  , 188686}, // capitulation
    {12737077, 188686}, // occupation

    // -> airstrike
    {838364 , 2380335}, // carpet bombing
    {4688003, 2380335}, // aerial bombing of a city

    // -> fire
    {3149858  , 3196}, // fire in Edo
    {168983   , 3196}, // conflagration
    {838718   , 3196}, // city fire
    {11396401 , 3196}, // train fire
    {18215814 , 3196}, // tunnel fire
    {116498219, 3196}, // library fire
    {350929   , 3196}, // vehicle fire
    {7081474  , 3196}, // oil well fire
    {7625093  , 3196}, // structure fire
    {17094485 , 3196}, // industrial fire

    // -> wildfire
    {83645467 , 169950}, // Megafire
    {107434304, 169950}, // forest fire
    {73712380 , 169950}, // wildfire season
    {113671616, 169950}, // wildfire season in Spain
    {113673971, 169950}, // wildfire season in Greece
    {107637456, 169950}, // wildfires in Catalonia
    {113674113, 169950}, // France wildfire season
    {113674173, 169950}, // Turkey wildfire season
    {113674231, 169950}, // Algeria wildfire season
    {107637129, 169950}, // Catalonia wildfire season
    {73712583 , 169950}, // regional wildfire season
    {114051407, 169950}, // Texas wildfire season
    {113673914, 169950}, // Oregon wildfire season
    {113680666, 169950}, // Nevada wildfire season
    {113680707, 169950}, // Arizona wildfire season
    {113673789, 169950}, // Colorado wildfire season
    {113671358, 169950}, // California wildfire season
    {113673861, 169950}, // Washington wildfire season
    {113680693, 169950}, // New Mexico wildfire season

    // -> coronation
    {39087739 , 209715}, // coronation of the Thai monarch
    {2615857  , 209715}, // coronation of the French monarch
    {464122   , 209715}, // coronation of the British monarch
    {834550   , 209715}, // coronation of the Russian monarch
    {105742802, 209715}, // coronation of the Byzantine emperor
    {48847692 , 209715}, // coronation of the Emperor of Brazil
    {318140   , 209715}, // coronation of the Holy Roman Emperor

    // -> train wreck
    {1331380 , 1078765}, // derailment
    {19403959, 1078765}, // railway accident
    {11396408, 1078765}, // train collision
    {19710423, 1078765}, // level crossing collision

    // -> public election
    {76853179, 40231}, // group of elections
    {858439  , 40231}, // presidential election
    {4128699 , 40231}, // Russian presidential election
    {22342957, 40231}, // Malian parliamentary election
    {22442768, 40231}, // Togolese parliamentary election

    // -> voting
    {1214249 , 189760}, // plebiscite
    {2354171 , 189760}, // straw poll
    {43109   , 189760}, // referendum
    {1021073 , 189760}, // local referendum
    {331513  , 189760}, // popular referendum
    {431226  , 189760}, // optional referendum
    {1219394 , 189760}, // independence referendum
    {2515494 , 189760}, // constitutional referendum
    {16674288, 189760}, // self-determination referendum
    {54086790, 189760}, // referendums in Ireland
    {16595070, 189760}, // referendum related to European Union accession

    // -> protest
    {1679887 , 273120}, // student protest
    {2255532 , 273120}, // silent protest
    {3109572 , 273120}, // civil resistance
    {47217   , 273120}, // civil disobedience
    {14523556, 273120}, // sit-in
    {72850604, 273120}, // street blockade
    {1562095 , 273120}, // human chain

    // -> strike
    {3118387  , 49776}, // sex strike
    {7209607  , 49776}, // police strike
    {780417   , 49776}, // school strike
    {116885608, 49776}, // teacher strike
    {49775    , 49776}, // general strike
    {1318839  , 49776}, // women's strike
    {7623077  , 49776}, // Streetcar strikes in the United States

    // -> nuclear explosion
    {4367188 , 2656967}, // underwater nuclear explosion
    {210112  , 2656967}, // nuclear weapons test
    {98391050, 2656967}, // nuclear test series
    {98607365, 2656967}, // atmospheric nuclear test
    {3058675 , 2656967}, // underground nuclear weapons test
    {3780403 , 2656967}, // high-altitude nuclear explosion
    {4382347 , 2656967}, // peaceful nuclear explosion
    {15142894, 2656967}, // weapon model

    // -> expedition
    {366301   , 2401485}, // research expedition
    {122861378, 2401485}, // research expedition series
    {111988676, 2401485}, // expedition series
    {109712283, 2401485}, // collecting camp
    {130475091, 2401485}, // oceanographic cruise

    // -> motorsport
    {5386, 5367}, // auto racing
    {6470596, 5367}, // Lady Wigram Trophy


    // -> transport accident
    {9687    , 11822042}, // traffic collision
    {61037771, 11822042}, // car collision
    {60895708, 11822042}, // bus accident
    {7833114 , 11822042}, // tram accident
    {62730040, 11822042}, // trolleybus accident

    // -> mining accident
    {23007305, 1550225}, // mine explosion
    {12089136, 1550225}, // firedamp explosion

    // -> aviation accident
    {3002150  , 744913}, // aircraft crash
    {26975538 , 744913}, // airplane crash
    {6539177  , 744913}, // airliner shootdown
    {8228287  , 744913}, // airliner shootdown incident
    {1863435  , 744913}, // mid-air collision
    {106466398, 744913}, // helicopter crash

    // -> aviation incident
    {1074121 , 3149875}, // emergency landing
    {17363717, 3149875}, // runway excursion

    // -> accident
    {629257  , 171558}, // work accident
    {11875820, 171558}, // sports accident
    {977367  , 171558}, // chemical accident

    // -> disaster
    {8065     , 3839081}, // natural disaster
    {15725976 , 3839081}, // nuclear disaster
    {42643444 , 3839081}, // stadium disaster
    {106470546, 3839081}, // climbing disaster
    {2620513  , 3839081}, // maritime disaster
    {3193890  , 3839081}, // environmental disaster
    {1309431  , 3839081}, // structural failure
    {1033074  , 3839081}, // dam failure
    {54643580 , 3839081}, // tailings dam failure
    {1817963  , 3839081}, // debris flow
    {109905701, 3839081}, // crowd collapses and crushes

    // -> meeting
    {7269307, 2761147}, // parliamentary meeting
    {4358176, 2761147}, // council
    {4228029, 2761147}, // colloquium
        
    // -> summit
    {643292   , 1072326}, // NATO summit
    {535086   , 1072326}, // Ibero-American Summit
    {28963415 , 1072326}, // G6 / G7 / G8 summit
    {28966106 , 1072326}, // G6 summit
    {28966115 , 1072326}, // G7 summit
    {20739115 , 1072326}, // G8 summit
    {26805735 , 1072326}, // G20 summit
    {30178840 , 1072326}, // SCO summit
    {20389574 , 1072326}, // Arab League summit
    {118501455, 1072326}, // China-Central Asia Summit

    // -> explosion
    {1362483, 179057}, // gas explosion
    {2296331, 179057}, // dust explosion
    {1417169, 179057}, // boiler explosion
    {7918434, 179057}, // vehicle explosion

    // -> crisis
    {1098831  , 381072}, // water crisis
    {3002772  , 381072}, // political crisis
    {1968864  , 381072}, // government crisis
    {112683179, 381072}, // social crisis
    {62994546 , 381072}, // socioeconomic crisis
    {380552   , 381072}, // constitutional crisis

    // -> financial risk
    {806729 , 1337875}, // banking crisis
    {290178 , 1337875}, // economic crisis
    {114380 , 1337875}, // financial crisis
    {3539169, 1337875}, // economic problem

    // -> blockade
    {1959141, 273976}, // naval blockade

    // -> assassination
    {1642263, 3882219}, // targeted killing

    // -> political murder
    {1475448 , 1139665}, // regicide
    {88178910, 1139665}, // assassination attempt

    {1006311, 21994376}, // war of national liberation -> war of independence
    {5533440, 41397}, // genocidal massacre -> genocide
    {263233, 1417098}, // United States presidential inauguration -> inauguration
    {63442071, 49836}, // royal wedding -> wedding
    {194465, 1361229}, // annexation -> conquest
    {876274, 1261499}, // naval warfare -> naval battle
    {1255828, 179875}, // controversy -> debate
    {27653727, 678146}, // naval bombing of a city -> bombardment
    
    // -> shipwrecking
    {30880545, 906512}, // sinking
    {852190  , 906512}, // shipwreck
    {2235325 , 906512}, // maritime accident
    {3362987 , 906512}, // four funnel liner

    {40728071, 421}, // UFO sighting -> unidentified flying object
    
    // -> coup
    {25906438, 45382}, // attempted coup
    {3449092 , 45382}, // military coup

    // -> flood
    {860333, 8068}, // flash flood
    {180382, 8068}, // cloud burst

    // -> murder
    {5300066 , 132821}, // double murder
    {16266378, 132821}, // serial murder
    {11487748, 132821}, // robbery-murder
    {81672   , 132821}, // attempted murder
    {4676786 , 132821}, // deliberate murder
    {3307578 , 132821}, // murder–suicide
    {844482  , 132821}, // killing
    {66975460, 132821}, // spree killing
    {3882220 , 132821}, // manslaughter
    {17450153, 132821}, // animal attack
    {2438548 , 132821}, // case of death
    {20730691, 132821}, // death in police custody

    // -> homicide
    {6317275 , 149086}, // justifiable homicide
    {11385965, 149086}, // insurance killing
    {1186609 , 132821}, // democide
    {13587273, 132821}, // pedicide
    {1136456 , 149086}, // matricide
    {5400895 , 149086}, // familicide
    {916412  , 132821}, // infanticide
    {61039291, 149086}, // knife attack
    {464643  , 149086}, // stabbing
    {6813020 , 149086}, // stabbing attack
    {64149164, 149086}, // mass stabbing
    {17142966, 149086}, // slashing
    {11547135, 132821}, // homicide in Penal code of Japan

    // -> terrorist attack
    {28945444, 2223653}, // failed attack
    {5429679 , 2223653}, // failed terrorism plot
    {25917186, 2223653}, // coordinated terrorist attack

    // -> legislature
    {2282872 , 11204}, // diet
    {24929187, 11204}, // legislature of the Parliament of Catalonia

    // -> congress
    {2993090, 2495862}, // SAT Congress

    // -> party conference
    {15908445, 2288051}, // National Congress of the Kuomintang
    {183482  , 2288051}, // National Congress of the Chinese Communist Party
    {7888355 , 2288051}, // United Nations Climate Change conference

    // -> shooting
    {111986723, 2252077}, // shot by the police
    {109217482, 2252077}, // shooting attack
    {1029371  , 2252077}, // active shooter
    {5510053  , 2252077}, // fusillade
    {5618454  , 2252077}, // gun violence

    // -> mass shooting
    {2406205 , 21480300}, // shootout
    {473853  , 21480300}, // school shooting
    {42915628, 21480300}, // mass shooting in the United States

    // -> police operation
    {97368680, 1707496}, // police raid

    // -> demonstration
    {19609158 , 175331}, // rally
    {60716473 , 175331}, // rally
    {1210697  , 175331}, // die-in
    {15631336 , 175331}, // protest march
    {69405214 , 175331}, // bicycle demonstration
    {124012559, 175331}, // Erster Mai Demonstration in Berlin
    {898830   , 175331}, // processional parade

    // -> car bombing
    {20893947, 25917154}, // suicide car bombing
    {61037469, 25917154}, // bus bombing

    // -> suicide attack
    {18493502, 217327}, // suicide bombing

    // -> suicide
    {24518 , 10737}, // cult suicide
    {142824, 10737}, // mass suicide

    // -> research
    {7315176, 42240}, // research program
    {101965 , 42240}, // experiment
    {1331083, 42240}, // human subject research project

    // -> robbery
    {106034988, 53706}, // jewel heist
    {7832904  , 53706}, // train robbery


    {1509831, 507850}, // title of Mary -> Marian apparition
    {1371150, 318296}, // hostage taking -> kidnapping
    {1756454, 2727213}, // art theft -> theft
    {934744, 18343076}, // political scandal -> corruption scandal
    {12761956, 192909}, // affair -> scandal
        
    // -> arson
    {897797  , 327541}, // arson attack
    {19903830, 327541}, // church arson


    {2696963, 8081}, // tornado outbreak -> tornado
    
    // -> sexual assault
    {43414  , 673281}, // sexual abuse
    {751722 , 673281}, // sexual harassment
    {47092  , 673281}, // rape
    {8192354, 673281}, // gang rape
    
    // -> cyclone
    {8092    , 79602}, // tropical cyclone
    {2331851 , 79602}, // subtropical cyclone
    {1063457 , 79602}, // extratropical cyclone
    {63182478, 79602}, // intense tropical cyclone
    {1781555 , 79602}, // Mediterranean tropical cyclone
    {2547976 , 79602}, // North Atlantic tropical cyclone
    {148942  , 79602}, // European windstorm
    {81054   , 79602}, // storm

    // -> hurricane
    {63100559, 34439356}, // Category 1 hurricane
    {63100584, 34439356}, // Category 2 hurricane
    {63100595, 34439356}, // Category 3 hurricane
    {63100601, 34439356}, // Category 4 hurricane
    {63100611, 34439356}, // Category 5 hurricane
    {18650988, 34439356}, // tropical depression
    {3157041 , 34439356}, // tropical storm
    {96053756, 34439356}, // tropical storm
    {3023711 , 34439356}, // derecho

    // -> Olympic Games
    {159821, 5389}, // Summer Olympic Games
    {82414 , 5389}, // Winter Olympic Games

    // -> vehicle ramming
    {18711682, 25893885}, // vehicle-ramming attack

    // -> assault
    {114905986, 365680}, // aggravated assault


    {1265353, 8016240}, // war crimes trial -> trial

    {60845736, 2334719}, // criminal law case -> legal case
    {205418, 304958}, // blizzard -> ice storm
    {2964429, 60186}, // L chondrite -> meteorite
    {100268926, 486972}, // iberian settlement -> human settlement
    {2324916, 11430552}, // state visit -> diplomatic visit
    {123488411, 168247} // Starvation -> famine
//     {, }, //  -> 
//     {, }, //  -> 
//     {, }, //  -> 
//     {, }, //  -> 
//     {, }, //  -> 
//     {, }, //  -> 
};

inline uint32_t remap_class(uint32_t c) noexcept
{
    auto it = classRemap.find(c);
    return (it != classRemap.end()) ? it->second : c;
}