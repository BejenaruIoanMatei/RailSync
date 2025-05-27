--drop table plecari;
--insert into plecari (id_plecare, ziua,statie,id_tren,destinatie,ora,intarziere) values ('A3','Luni','Iasi','IR567','Mangalia',7,0);
--insert into plecari (id_plecare, ziua,statie,id_tren,destinatie,ora,intarziere) values ('A4','Luni','Bucuresti','IR567','Mangalia',13,0);
--insert into plecari (id_plecare, ziua,statie,id_tren,destinatie,ora,intarziere) values ('A5','Luni','Iasi','IR433','Vaslui',19,0);
--insert into plecari (id_plecare, ziua,statie,id_tren,destinatie,ora,intarziere) values ('B1','Marti','Iasi','IR433','Vaslui',19,0);
--insert into plecari (id_plecare, ziua,statie,id_tren,destinatie,ora,intarziere) values ('B2','Marti','Iasi','IR433','Cluj',13,0);
--insert into plecari (id_plecare, ziua,statie,id_tren,destinatie,ora,intarziere) values ('B3','Marti','Vaslui','IR433','Cluj',15,0);
---Luni--iasi  -> bucuresti  -> mangalia
---Marti--iasi  -> vaslui     -> cluj
--select * from plecari;
--select * from plecari;

insert into sosiri (id_sosire, ziua,unde_soseste,id_tren,de_unde_soseste,ora,intarziere) values ('SOS2','Luni','Cluj','IR567','Bucuresti',23,10);
--insert into sosiri (id_sosire, ziua,unde_soseste,id_tren,de_unde_soseste,ora,intarziere) values ('X4','Marti','Vaslui','IR433','Iasi',21,0);
--insert into sosiri (id_sosire, ziua,unde_soseste,id_tren,de_unde_soseste,ora,intarziere) values ('X6','Duminica','Iasi','IR433','Cluj',23,0);



--select * from plecari;
select * from sosiri;
--select * from useri;
--drop table sosiri;
--drop table plecari;
--drop table useri;

--select * from sosiri s
--join useri u on s.unde_soseste = u.unde_soseste;

