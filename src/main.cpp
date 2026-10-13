#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
struct Game {
  std::string room = "start";
  int lives = 5, score = 0;
  int hall = 0;
  bool bedroom = false, kitchen = false, library = false, code = false, won = false;
  std::string message;
};
std::unordered_map<std::string, Game> games;

std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
  return s;
}
std::string decode(const std::string& s) {
  std::string out;
  for (size_t i=0;i<s.size();++i) {
    if (s[i]=='+') out+=' ';
    else if (s[i]=='%' && i+2<s.size()) { out+=static_cast<char>(std::strtol(s.substr(i+1,2).c_str(),nullptr,16)); i+=2; }
    else out+=s[i];
  }
  return out;
}
std::map<std::string,std::string> fields(const std::string& body) {
  std::map<std::string,std::string> out; std::stringstream ss(body); std::string part;
  while(std::getline(ss,part,'&')) { auto p=part.find('='); out[decode(part.substr(0,p))]=p==std::string::npos?"":decode(part.substr(p+1)); }
  return out;
}
std::string esc(const std::string& s) {
  std::string o; for(char c:s) { if(c=='&')o+="&amp;"; else if(c=='<')o+="&lt;"; else if(c=='>')o+="&gt;"; else if(c=='\"')o+="&quot;"; else o+=c; } return o;
}
bool hallDone(const Game& g){ return g.hall>=3; }
bool allowed(const Game& g,const std::string& r) {
  if(r=="hall") return g.room!="start";
  if(r=="bedroom") return hallDone(g);
  if(r=="kitchen") return g.bedroom;
  if(r=="library") return g.kitchen;
  if(r=="stone") return g.library;
  if(r=="treasure") return g.code;
  return false;
}
std::string label(const std::string& r) {
  if(r=="start")return "Ravenwood Estate"; if(r=="hall")return "The Grand Hall";
  if(r=="bedroom")return "The Cursed Bedroom"; if(r=="kitchen")return "The Haunted Kitchen";
  if(r=="library")return "The Forbidden Library"; if(r=="stone")return "The Stone Door";
  if(r=="treasure")return "The Treasure Chamber"; if(r=="victory")return "The Last Secret"; return "Game Over";
}
std::string button(const std::string& text,const std::string& name,const std::string& value) {
  return "<button class='button' name='"+name+"' value='"+value+"'>"+text+"</button>";
}
std::string door(const Game& g,const std::string& dest,const std::string& title,const std::string& locked) {
  bool open=allowed(g,dest);
  return "<form method='post' class='door-form'><button class='door "+std::string(open?"open":"locked")+"' name='door' value='"+dest+"'><span class='door-symbol'>"+(open?"✧":"⚿")+"</span><strong>"+title+"</strong><small>"+(open?"ENTER":"LOCKED · "+locked)+"</small></button></form>";
}
std::string page(const Game& g) {
  std::string scene="scene-"+g.room;
  std::ostringstream h;
  h<<"<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Secrets of Ravenwood</title><link rel='stylesheet' href='/style.css'></head><body class='"<<scene<<"'>";
  h<<"<main class='frame'><header><a class='brand' href='/'>SECRETS <i>OF</i> RAVENWOOD</a><div class='stats'><span>♥ "<<g.lives<<" lives</span><span>✦ "<<g.score<<" points</span></div></header>";
  if(!g.message.empty())h<<"<div class='notice'>"<<esc(g.message)<<"</div>";
  if(g.lives<=0) {
    h<<"<section class='intro'><p class='eyebrow'>THE HOUSE CLAIMS ANOTHER</p><h1>Game Over</h1><p>Your lantern has gone dark. Ravenwood keeps its secrets.</p><form method='post'>"<<button("RESTART THE NIGHT","action","restart")<<"</form></section>";
  } else if(g.room=="start") {
    h<<"<section class='intro'><p class='eyebrow'>AN INTERACTIVE GOTHIC MYSTERY</p><h1>Some doors<br>remember.</h1><p>At the edge of the moor, Ravenwood waits beneath a moonless sky. Find what the family hid before the house finds you.</p><form method='post'>"<<button("ENTER THE MANSION","action","start")<<"</form><p class='subtle'>A candle. Five lives. One night to uncover the truth.</p></section>";
  } else if(g.room=="victory"||g.won) {
    h<<"<section class='intro'><p class='eyebrow'>THE CURSE IS BROKEN</p><h1>The house<br>is quiet.</h1><p>The chest opens. Beneath the gold lies a letter bearing your own name. Dawn spills across the stone floor.</p><form method='post'>"<<button("PLAY AGAIN","action","restart")<<"</form></section>";
  } else {
    h<<"<section class='room-head'><p class='eyebrow'>RAVENWOOD ESTATE · "<<g.score<<" SOULS FOUND</p><h1>"<<label(g.room)<<"</h1></section><section class='scene'><div class='scene-caption'>";
    if(g.room=="hall") h<<"Dust moves in the candlelight. Three riddles are carved into the banister.";
    if(g.room=="bedroom") h<<"The mirror fogs from the inside. A word crawls across its silvered glass.";
    if(g.room=="kitchen") h<<"A chef's translucent hand points toward a bowl of ash and herbs.";
    if(g.room=="library") h<<"The books whisper numbers. One spine is warm to the touch.";
    if(g.room=="stone") h<<"Four ancient symbols glow faintly in the sealed stone.";
    if(g.room=="treasure") h<<"Gold glints beneath a shaft of moonlight. An ironbound chest rests at the center.";
    h<<"</div></section><section class='play'>";
    if(g.room=="hall") {
      const char* qs[]={"I have a face and two hands, but no arms. What am I?","The more you take, the more you leave behind. What are they?","I speak without a mouth and hear without ears. What am I?"};
      if(!hallDone(g)){int i=g.hall; h<<"<p class='eyebrow'>RIDDLE "<<i+1<<" OF 3</p><h2>"<<qs[i]<<"</h2><form method='post' class='answer'><input name='answer' placeholder='Your answer…' required><button class='button'>SUBMIT ANSWER</button></form><details><summary>Ask the house for a hint</summary><p>"<<(i==0?"Look for it on a wall.":i==1?"They follow every traveler.":"It returns what it hears.")<<"</p></details>"; }
      else h<<"<h2>The staircase yields its secret.</h2><p>Beyond the portraits, the bedroom door stands ajar.</p>";
      h<<door(g,"bedroom","Bedroom","solve all 3 hall riddles");
      if(!hallDone(g))h<<"<p class='subtle'>"<<g.hall<<" / 3 riddles solved</p>";
    } else if(g.room=="bedroom") {
      if(!g.bedroom)h<<"<p class='eyebrow'>MIRROR SCRAMBLE</p><h2>Rearrange: <span class='rune'>R O O D C L</span></h2><form method='post' class='answer'><input name='answer' placeholder='The word…' required><button class='button'>SUBMIT ANSWER</button></form><details><summary>Ask the house for a hint</summary><p>It keeps your clothes behind its doors.</p></details>";
      else h<<"<h2>The mirror clears.</h2><p>A cold draft rises from the kitchen below.</p>";
      h<<door(g,"kitchen","Kitchen","solve the mirror puzzle");
    } else if(g.room=="kitchen") {
      if(!g.kitchen)h<<"<p class='eyebrow'>THE CHEF'S LAST MEAL</p><h2>What grows underground, has a papery skin, and makes ghosts weep?</h2><form method='post' class='answer'><input name='answer' placeholder='Name the ingredient…' required><button class='button'>SUBMIT ANSWER</button></form><details><summary>Ask the house for a hint</summary><p>It keeps vampires at a distance.</p></details>";
      else h<<"<h2>The chef bows and fades.</h2><p>In the recipe book, a library key waits.</p>";
      h<<door(g,"library","Library","solve the chef's mystery");
    } else if(g.room=="library") {
      if(!g.library)h<<"<p class='eyebrow'>THE ASTRONOMER'S NOTE</p><h2>How many moons circle Earth?</h2><form method='post' class='answer'><input name='answer' placeholder='A number…' required><button class='button'>SUBMIT ANSWER</button></form><details><summary>Inspect the star chart</summary><p>Earth has one natural satellite. The clue is also the first digit of the door code.</p></details>";
      else h<<"<h2>A brass token falls from the atlas.</h2><p>The library clue: the year the estate was built is 1896. The moon count begins the code.</p>";
      h<<door(g,"stone","Secret Stone Door","collect the library clue");
    } else if(g.room=="stone") {
      h<<"<p class='eyebrow'>FOUR DIGITS · CLUES FROM THE HOUSE</p><p class='subtle'>Moon · number of hall riddles · number of kitchen fires · final digit of the founding year</p>";
      if(!g.code)h<<"<form method='post' class='answer'><input name='answer' inputmode='numeric' maxlength='4' placeholder='Four-digit code' required><button class='button'>UNSEAL THE DOOR</button></form>";
      else h<<"<h2>The symbols flare. The stone door withdraws.</h2>";
      h<<door(g,"treasure","Treasure Chamber","enter the correct code");
    } else if(g.room=="treasure") {
      h<<"<p class='eyebrow'>THE LAST SECRET</p><h2>The ironbound chest is within reach.</h2><form method='post'>"<<button("OPEN THE TREASURE CHEST","action","chest")<<"</form>";
    }
    h<<"</section><footer><span>THE NIGHT HAS "<<g.lives<<" BREATHS LEFT</span><form method='post'>"<<button("RESTART","action","restart")<<"</form></footer>";
  }
  h<<"</main></body></html>"; return h.str();
}
}
int main(int argc,char**argv) {
  int port=argc>1?std::atoi(argv[1]):8080; int server=socket(AF_INET,SOCK_STREAM,0); int yes=1; setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes);
  sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_addr.s_addr=INADDR_ANY; addr.sin_port=htons(port);
  if(bind(server,reinterpret_cast<sockaddr*>(&addr),sizeof addr)<0||listen(server,32)<0){perror("server");return 1;}
  std::cout<<"Ravenwood listening on http://localhost:"<<port<<"\n";
  while(true) {
    int client=accept(server,nullptr,nullptr); if(client<0)continue;
    std::string req; char buf[4096]; int n=recv(client,buf,sizeof(buf),0); if(n<=0){close(client);continue;} req.assign(buf,n);
    auto end=req.find("\r\n\r\n"); if(end==std::string::npos){close(client);continue;}
    std::string head=req.substr(0,end), body=req.substr(end+4); std::string cookie;
    auto cp=head.find("Cookie:"); if(cp!=std::string::npos){auto c=head.find("sid=",cp);if(c!=std::string::npos){c+=4;auto e=head.find_first_of("; \r\n",c);cookie=head.substr(c,e-c);}}
    if(cookie.empty()){cookie=std::to_string(reinterpret_cast<uintptr_t>(&games)^static_cast<uintptr_t>(client)^static_cast<uintptr_t>(std::rand()));}
    size_t lenpos=head.find("Content-Length:"); size_t contentLen=0; if(lenpos!=std::string::npos)contentLen=std::strtoul(head.c_str()+lenpos+15,nullptr,10);
    while(body.size()<contentLen){n=recv(client,buf,sizeof(buf),0);if(n<=0)break;body.append(buf,n);}
    Game& g=games[cookie]; auto fieldsMap=fields(body); std::string response;
    if(head.find("GET /style.css ")!=std::string::npos) {
      std::ifstream file("style.css",std::ios::binary);std::stringstream data;data<<file.rdbuf();auto css=data.str();
      std::string out="HTTP/1.1 200 OK\r\nContent-Type: text/css; charset=utf-8\r\nContent-Length: "+std::to_string(css.size())+"\r\nConnection: close\r\n\r\n"+css;send(client,out.data(),out.size(),0);
    } else if(head.find("POST ")!=std::string::npos) {
      if(fieldsMap.count("action")&&fieldsMap["action"]=="restart"){g=Game{};g.message="The clock turns back. Ravenwood is waiting.";}
      else if(fieldsMap.count("action")&&fieldsMap["action"]=="start"){g.room="hall";g.message="The heavy door closes behind you.";}
      else if(fieldsMap.count("door")){auto d=fieldsMap["door"];if(allowed(g,d)){g.room=d;g.message="You cross the threshold into "+label(d)+".";}else g.message="The door is locked. Complete this room's challenge first.";}
      else if(fieldsMap.count("answer")){
        auto a=lower(fieldsMap["answer"]); a.erase(std::remove_if(a.begin(),a.end(),[](unsigned char c){return std::isspace(c);}),a.end());
        bool ok=false;
        if(g.room=="hall"&&g.hall<3){const std::string answers[]={"clock","footsteps","echo"};ok=a==answers[g.hall];if(ok){++g.hall;g.score+=100;g.message="Correct. A candle flares to life.";}}
        else if(g.room=="bedroom"&&!g.bedroom){ok=a=="closet"||a=="wardrobe";if(ok){g.bedroom=true;g.score+=150;g.message="The mirror clears. The kitchen door unlocks.";}}
        else if(g.room=="kitchen"&&!g.kitchen){ok=a=="onion"||a=="garlic";if(ok){g.kitchen=true;g.score+=150;g.message="The chef's mystery is solved. The library door unlocks.";}}
        else if(g.room=="library"&&!g.library){ok=a=="1"||a=="one";if(ok){g.library=true;g.score+=200;g.message="Clue collected: moon = 1. The stone door unlocks.";}}
        else if(g.room=="stone"&&!g.code){ok=a=="1336";if(ok){g.code=true;g.score+=250;g.message="The code is accepted. The treasure chamber opens.";}}
        if(!ok){--g.lives;g.message=g.lives>0?"The house rejects your answer. One life lost.":"Your final breath fades in the dark.";}
      } else if(fieldsMap.count("action")&&fieldsMap["action"]=="chest"&&g.room=="treasure"){g.won=true;g.room="victory";g.score+=500;g.message="You have uncovered the final secret.";}
      std::string out="HTTP/1.1 303 See Other\r\nLocation: /\r\nSet-Cookie: sid="+cookie+"; Path=/; HttpOnly; SameSite=Lax\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";send(client,out.data(),out.size(),0);
    } else {
      if(g.room=="start") {std::ifstream file("index.html",std::ios::binary);std::stringstream data;data<<file.rdbuf();response=data.str();}
      else response=page(g);
      std::ostringstream out;out<<"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nSet-Cookie: sid="<<cookie<<"; Path=/; HttpOnly; SameSite=Lax\r\nContent-Length: "<<response.size()<<"\r\nConnection: close\r\n\r\n"<<response;auto s=out.str();send(client,s.data(),s.size(),0);
    }
    close(client);
  }
}
