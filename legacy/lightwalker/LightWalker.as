// ActionScript 2 frame scripts extracted from LightWalker.fla (April 2014), in timeline order.

// ---- Frame script 1 ----
//www.youtube.com/watch?v=Rpl2-r8ybGo

// ---- Frame script 2 ----
stop();

//function linked to a button that starts the game
btn_play.onPress = function(){
	gotoAndStop("gameon");
}

//function linked to a button that loads the instructions
btn_intro.onPress = function(){
	gotoAndStop("intro");
}

//www.youtube.com/watch?v=ePDN3Mde_oY

// ---- Frame script 3 ----
stop();

myData = new LoadVars();
myData.load("get_score.php");

myData.onLoad = function(success){
	if(success){
		for(var i=0; i<this('count'); i++){
			_root["n"+i].text = this['name' + i];
			_root["s"+i].text = this['score' + i];
		}
	}
	else {
		trace("Error loading data!");
	}
}

// ---- Frame script 4 ----
//function linked to a button that starts the game
btn_play.onPress = function(){
	gotoAndStop("gameon");
}

//function linked to a button that returns to the previous menu
btn_back.onPress = function(){
	gotoAndStop("load");
}

// ---- Frame script 5 ----
stop();

init(); //calls the init function

//function for the basic functionality of the game
function init(){
	LWDEPTH = 30000;
	TOPBOUND = 60; //top boundary
	BOTTOMBOUND = 473; //bottom boundary
	RIGHTBOUND = 626; //right boundary
	LEFTBOUND = 22; //left boundary
	
	ENEMYDEPTH =50;
	ENEMYSPEED = 6;
	ENEMYMOVECLOCK = 220;
	DAJJALMOVECLOCK = 300;
	
	SOUNDDEPTH = 300;
	
	GOUPFLAG = false;
	GODOWNFLAG = false;
	GORIGHTFLAG = false;
	GOLEFTFLAG = false;
	
	myListener = new Object();
	Key.addListener(myListener);
	myListener.onKeyUp = LWKeyUp;
	myListener.onKeyDown = LWKeyDown;
	
	SHOTTIMER = 0;
	LASERCOUNT = 1;
	BULLETSPEED = 10;
	
	GAMELEVEL = 1;
	scoreboard.lvl.text = GAMELEVEL;
	SCORE = 0;
	LIVES = 3;
	livesboard.gotoAndStop(4);
	
	createLW();
	createEnemies();
	createSoundManager();
	
	moveEnemyInterval = setInterval(moveEnemies,ENEMYMOVECLOCK);
	moveEnemyInterval = setInterval(moveDajjal,DAJJALMOVECLOCK);
}

//function to implement LW onto the stage
function createLW(){
	_root.attachMovie("LW","LW",LWDEPTH,{_x:Stage.width/2-6,_y:Stage.height/2-13})
	LW.onEnterFrame = LWenterFrame;
}

//function for LW entering the frame
function LWenterFrame(){
	//directions when keys are pressed
	if(Key.isDown(38)){
		LW._y -= 6;
		if(LW._y <= TOPBOUND){
		LW._y = TOPBOUND;
	   }
	}
	else if(Key.isDown(40)){
	   LW._y += 6;
	   if(LW._y >= BOTTOMBOUND){
		  LW._y = BOTTOMBOUND;
	   }
	}
	if(Key.isDown(37)){
	   LW._x -= 6;
	   if(LW._x <= LEFTBOUND){
		   LW._x  = LEFTBOUND;
	   }
	}
	else if(Key.isDown(39)){
	   LW._x += 6;
	   if(LW._x >= RIGHTBOUND){
		   LW._x  = RIGHTBOUND;
	   }
	}
	
	if (Key.isDown(87) && Key.isDown(68) && SHOTTIMER <= 0){
		createLaser(135,"upRight");
		SHOTTIMER = 7;
	}
	else if (Key.isDown(87) && Key.isDown(65) && SHOTTIMER <= 0){
		createLaser(45,"upLeft");
		SHOTTIMER = 7;
	}
	else if (Key.isDown(83) && Key.isDown(68) && SHOTTIMER <= 0){
		createLaser(45,"downRight");
		SHOTTIMER = 7;
	}
	else if (Key.isDown(83) && Key.isDown(65) && SHOTTIMER <= 0){
		createLaser(135,"downLeft");
		SHOTTIMER = 7;
	}
	else if (Key.isDown(87) && SHOTTIMER <= 0){
		createLaser(90,"up");
		SHOTTIMER = 7;
	}
	else if (Key.isDown(83) && SHOTTIMER <= 0){
		createLaser(90,"down");
		SHOTTIMER = 7;
	}
	else if (Key.isDown(65) && SHOTTIMER <= 0){
		createLaser(0,"left");
		SHOTTIMER = 7;
	}
	else if (Key.isDown(68) && SHOTTIMER <= 0){
		createLaser(0,"right");
		SHOTTIMER = 7;
	}
	SHOTTIMER--;
}

//function that creates the LW's laser
function createLaser(rotate, direction){
	attachMovie("laser","laser"+LASERCOUNT,30000+LASERCOUNT, {_x:LW._x,_y:LW._y});
	myLaser = "laser"+LASERCOUNT;
	_root[myLaser]._rotation += rotate;
	_root[myLaser].direction = direction;
	LASERCOUNT++;
	soundManager.laser.mySound.start();
	_root[myLaser].onEnterFrame = moveLaser;
}

//function to move the laser
function moveLaser(){
	
	//for loop for enemy collision
	for(index = 0; index < enemyArray.length; index++){
		en = enemyArray[index];
		if ((this._x < en._x+7.5 && this._x > en._x-7.5)&&
			(this._y <en._y+11.5 && this._y > en._y-11.5)){
				en.removeMovieClip();
				this.removeMovieClip();
				enemy.ENEMYCOUNT--;
				SCORE+=100
				scoreboard.score.text = SCORE;
			}
	}
	
	//levelling up
		if (enemy.ENEMYCOUNT == 0){
				GAMELEVEL++;
				scoreboard.lvl.text = GAMELEVEL;
				LW.removeMovieClip();
				delete LW.onEnterFrame;
				delete enemy.onEnterFrame;
				soundManager.spawn.mySound.start();
				for(k=1;k<=LASERCOUNT;k++){
					rmlaser = _root["laser"+k];
					delete rmlaser.onEnterFrame;
					rmlaser.removeMovieClip();
				}
				ENEMYSPEED = 6;
				newLevelInterval = setInterval(LWUpLevel,4000);
			}
		else if(enemy.ENEMYCOUNT == 13){
				ENEMYSPEED = 10;
			}
		else if(enemy.ENEMYCOUNT == 7){
				ENEMYSPEED = 15;
			}
	
	if(this.direction == "upRight"){
		this._x += BULLETSPEED;
		this._y -= BULLETSPEED;
		if(this._x >= RIGHTBOUND || this._y <= TOPBOUND){
			delete this.onEnterFrame;
			this.removeMovieClip();
		}
	}
	else if(this.direction == "upLeft"){
		this._x -= BULLETSPEED;
		this._y -= BULLETSPEED;
		if(this._x <= LEFTBOUND || this._y <= TOPBOUND){
			delete this.onEnterFrame;
			this.removeMovieClip();
		}
	}
	else if(this.direction == "downRight"){
		this._x += BULLETSPEED;
		this._y += BULLETSPEED;
		if(this._x >= RIGHTBOUND || this._y >= BOTTOMBOUND){
			delete this.onEnterFrame;
			this.removeMovieClip();
		}
	}
	else if(this.direction == "downLeft"){
		this._x -= BULLETSPEED;
		this._y += BULLETSPEED;
		if(this._x <= LEFTBOUND || this._y >= BOTTOMBOUND){
			delete this.onEnterFrame;
			this.removeMovieClip();
		}
	}
	
	else if(this.direction == "up"){
		this._y -= BULLETSPEED;
		if(this._y <= TOPBOUND){
			delete this.onEnterFrame;
			this.removeMovieClip();
		}
	}
	else if(this.direction == "down"){
		this._y += BULLETSPEED;
		if(this._y >= BOTTOMBOUND){
			delete this.onEnterFrame;
			this.removeMovieClip();
		}
	}
	else if(this.direction == "left"){
		this._x -= BULLETSPEED;
		if(this._y <= LEFTBOUND){
			delete this.onEnterFrame;
			this.removeMovieClip();
		}
	}
	else if(this.direction == "right"){
		this._x += BULLETSPEED;
		if(this._x >= RIGHTBOUND){
			delete this.onEnterFrame;
			this.removeMovieClip();
		}
	}
}

//function to play diff frames when specifci keys are pressed
function LWKeyDown(){
	k = Key.getCode(); 
	
	if (k == 37){
		if(GOLEFTFLAG == false){
			LW.gotoAndPlay("goleft");
			GOLEFTFLAG = true;
			GORIGHTFLAG = false;
			GOUPFLAG = false;
			GODOWNFLAG = false;
		}
	}
	else if (k == 39){
		if(GORIGHTFLAG == false){
			LW.gotoAndPlay("goright");
			GORIGHTFLAG = true;
			GOLEFTFLAG = false;
			GOUPFLAG = false;
			GODOWNFLAG = false;
		}
	}
	else if (k == 38){
		if(GOUPFLAG == false){
			LW.gotoAndPlay("goup");
			GOUPFLAG = true;
			GORIGHTFLAG = false;
			GOLEFTFLAG = false;
			GODOWNFLAG = false;
		}
	}
	else if (k == 40){
		if(GODOWNFLAG == false){
			LW.gotoAndPlay("godown");
			GODOWNFLAG = true;
			GORIGHTFLAG = false;
			GOUPFLAG = false;
			GOLEFTFLAG = false;
		}
	}
}

//function to set frame when keys are not pressed
function LWKeyUp(){
	if(GOUPFLAG == true){
		LW.gotoAndStop("goup");
	}
	else if(GODOWNFLAG == true){
		LW.gotoAndStop("godown");
	}
	else if(GORIGHTFLAG == true){
		LW.gotoAndStop("goright");
	}
	else if(GOLEFTFLAG == true){
		LW.gotoAndStop("goleft");
	}
}

//function to spawn enemies on the stage
function createEnemies(){
	createEmptyMovieClip("enemy",ENEMYDEPTH);
	enemy.ENEMYCOUNT = 0;
	enemy.onEnterFrame = enemyHitTest;
	
	
	enemyArray = new Array();
	
	i=GAMELEVEL;
	switch(i){
		case 1:
			for(j=0;j<15;j++){
				enemyArray.push(enemy.attachMovie("enemy","enemy"+enemy.ENEMYCOUNT,ENEMYDEPTH+enemy.ENEMYCOUNT++));
				
				MYENEMY = enemy["enemy"+enemy.ENEMYCOUNT];
				for(jj=0; jj<14;jj++){
					LOCX = Math.random()*500+20;
					LOCY = Math.random()*350+50;
					if((LOCX<220 || LOCX>430) && (LOCY<160 || LOCY>360)){
						MYENEMY._x = LOCX;
						MYENEMY._y = LOCY;
						break;
					}
				}
			}
		break;
		case 2:
		ENEMYSPEED = 8;
			for(j=0;j<35;j++){
				if(j<30){
					enemyArray.push(enemy.attachMovie("enemy","enemy"+enemy.ENEMYCOUNT,ENEMYDEPTH+enemy.ENEMYCOUNT++));
				
					MYENEMY = enemy["enemy"+enemy.ENEMYCOUNT];
					for(jj=0; jj<14;jj++){
						LOCX = Math.random()*500+20;
						LOCY = Math.random()*350+50;
						if((LOCX<220 || LOCX>430) && (LOCY<160 || LOCY>360)){
							MYENEMY._x = LOCX;
							MYENEMY._y = LOCY;
							break;
						}
					}
				}
				else{
					enemyArray.push(enemy.attachMovie("enemy","enemy"+enemy.ENEMYCOUNT,ENEMYDEPTH+enemy.ENEMYCOUNT++));
				
					MYENEMY = enemy["enemy"+enemy.ENEMYCOUNT];
					MYENEMY.gotoAndPlay("dajjal")
					for(jj=0; jj<14;jj++){
						LOCX = Math.random()*500+20;
						LOCY = Math.random()*350+50;
						if((LOCX<220 || LOCX>430) && (LOCY<160 || LOCY>360)){
							MYENEMY._x = LOCX;
							MYENEMY._y = LOCY;
							break;
						}
					}
				}
			}
		break;
		case 3:
		gotoAndStop("win");
		soundManager.clock.mySound.start();
		LW.removeMovieClip();
	}
}

//function to make enemies move
function moveEnemies(){
	if(GAMELEVEL==1){
		for(k=0; k<=enemyArray.length; k++){
			if(enemy["enemy"+k]._x < LW._x){
				enemy["enemy"+k]._x += ENEMYSPEED;
					if(enemy["enemy"+k]._y < LW._y){
					enemy["enemy"+k]._y += ENEMYSPEED;
					}
					else if(enemy["enemy"+k]._y > LW._y){
					enemy["enemy"+k]._y -= ENEMYSPEED;
					}
			}
		
			else if(enemy["enemy"+k]._x > LW._x){
				enemy["enemy"+k]._x -= ENEMYSPEED;
					if(enemy["enemy"+k]._y < LW._y){
					enemy["enemy"+k]._y += ENEMYSPEED;
					}
					else if(enemy["enemy"+k]._y > LW._y){
					enemy["enemy"+k]._y -= ENEMYSPEED;
					}
			}
		}
	}
	else if (GAMELEVEL==2){
		for (k=1; k<31; k++){
			if(enemy["enemy"+k]._x < LW._x){
				enemy["enemy"+k]._x += ENEMYSPEED;
					if(enemy["enemy"+k]._y < LW._y){
					enemy["enemy"+k]._y += ENEMYSPEED;
					}
					else if(enemy["enemy"+k]._y > LW._y){
					enemy["enemy"+k]._y -= ENEMYSPEED;
					}
			}
		
			else if(enemy["enemy"+k]._x > LW._x){
				enemy["enemy"+k]._x -= ENEMYSPEED;
					if(enemy["enemy"+k]._y < LW._y){
					enemy["enemy"+k]._y += ENEMYSPEED;
					}
					else if(enemy["enemy"+k]._y > LW._y){
					enemy["enemy"+k]._y -= ENEMYSPEED;
					}
			}
		}
	}
}

//function to move second enemy and make sure they don't run off stage
DR = 6;
DL = -6;
DU = -6
DD = 6;

function moveDajjal(){
	if(GAMELEVEL==2){
		for(k=30; k<=35; k++){
			if(!enemy["enemy"+k].aDir){
			   myRandomNum = Math.floor(Math.random()*5)
			   if(myRandomNum>=3){
				   enemy["enemy"+k].aDir = "right";
				   enemy["enemy"+k]._x += DR;
			   }
			   if(myRandomNum>=3){
				   enemy["enemy"+k].aDir = "left";
				   enemy["enemy"+k]._x += DL;
			   }
			   if(myRandomNum>=3){
				   enemy["enemy"+k].aDir = "up";
				   enemy["enemy"+k]._y += DU;
			   }
			   if(myRandomNum>=3){
				   enemy["enemy"+k].aDir = "down";
				   enemy["enemy"+k]._y += DD;
			   }
			}
			else{
				if(enemy["enemy"+k].aDir == "right"){
					if(enemy["enemy"+k]._x >=RIGHTBOUND){
						enemy["enemy"+k].aDir= "left";
						enemy["enemy"+k]._x += DL;
					}
					else{
						enemy["enemy"+k]._x += DR;
					}
				}
				else if(enemy["enemy"+k].aDir == "left"){
					if(enemy["enemy"+k]._x <=LEFTBOUND){
						enemy["enemy"+k].aDir= "right";
						enemy["enemy"+k]._x += DR;
					}
					else{
					enemy["enemy"+k]._x += DL;
					}
				}
				else if(enemy["enemy"+k].aDir == "up"){
					if(enemy["enemy"+k]._y <=TOPBOUND){
						enemy["enemy"+k].aDir= "down";
						enemy["enemy"+k]._y += DD;
					}
					else{
					enemy["enemy"+k]._y += DU;
					}
				}
				else if(enemy["enemy"+k].aDir == "down"){
					if(enemy["enemy"+k]._y >=BOTTOMBOUND){
						enemy["enemy"+k].aDir= "up";
						enemy["enemy"+k]._y += DU;
					}
					else{
					enemy["enemy"+k]._y += DD;
					}
				}
			}
		}
	}
}

//function for hitting enemies
function enemyHitTest(){
	for(k=1; k <= enemyArray.length; k++){
		if((this["enemy"+k]._x < LW._x+6.5) && (this["enemy"+k]._x > LW._x-6.5) &&
		   (this["enemy"+k]._y < LW._y+11.5) && (this["enemy"+k]._y > LW._y-11.5)){
			   pacifyLW();
		   }
	}
}

//function for when LW dies
function pacifyLW(){
	delete LW.onEnterFrame;
	LW.removeMovieClip();
	delete enemy.onEnterFrame;
	soundManager.pacify.mySound.start();
	
	LIVES--;
	livesboard.gotoAndStop(LIVES+1);
	
	LWInterval = setInterval(LWResetTimer,3000);
	
	if (LIVES==0){
		clearInterval(LWInterval);
		gotoAndStop("gameover");
		enemy.removeMovieClip();
	}
}

//function to reset LW in a safe place
function LWResetTimer(){
	createLW();
	BADPLACE = true;
	
	for(i=1;i<35;i++){
		if (BADPLACE==true){
			posx = Math.random()*500+20;
			posy = Math.random()*350+50;
			
			BADPLACE = false;
			for(ii=1; ii<=enemyArray.length; ii++){
				if(posx < enemyArray[ii]._x+90 && posx > enemyArray[ii]._x-90 &&
				   posy < enemyArray[ii]._y+90 && posy > enemyArray[ii]._y-90){
					   BADPLACE = true;
				   }
			}
		}
		if (BADPLACE==false){
			LW._x = posx;
			LW._y = posy;
		}
	}
	enemy.onEnterFrame = enemyHitTest;
	clearInterval(LWInterval);
}

//function to level up
function LWUpLevel(){
	createLW();
	createEnemies();
	clearInterval(newLevelInterval);
}

//function to contain sounds within the game
function createSoundManager(){
	createEmptyMovieClip("soundManager",SOUNDDEPTH);
	soundManager.createEmptyMovieClip("laser",SOUNDDEPTH++);
	soundManager.laser.mySound = new Sound(soundManager.laser);
	soundManager.laser.mySound.attachSound("laser_sound");
	soundManager.laser.mySound.setVolume(10);
	
	soundManager.createEmptyMovieClip("spawn",SOUNDDEPTH++);
	soundManager.spawn.mySound = new Sound(soundManager.spawn);
	soundManager.spawn.mySound.attachSound("spawn_sound");
	
	soundManager.createEmptyMovieClip("pacify",SOUNDDEPTH++);
	soundManager.pacify.mySound = new Sound(soundManager.pacify);
	soundManager.pacify.mySound.attachSound("pacify");
	
	soundManager.createEmptyMovieClip("clock",SOUNDDEPTH++);
	soundManager.clock.mySound = new Sound(soundManager.clock);
	soundManager.clock.mySound.attachSound("clock");
}



//www.youtube.com/playlist?list=PL9884A8978C9A7522

// ---- Frame script 6 ----
stop();

//function for the button that restarts the game
btn_restart.onPress = function(){
	gotoAndStop("gameon")
}

//function for the button that returns to the main menu
btn_main.onPress = function(){
	gotoAndStop("load");
	
}

// ---- Frame script 7 ----
stop();

//function for the button that returns to the main menu
btn_main.onPress = function(){
	gotoAndStop("load");
}
