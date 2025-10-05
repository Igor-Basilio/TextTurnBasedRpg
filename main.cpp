
#include <iostream>
#include <vector>
#include <unordered_map>
#include <list>
#include <assert.h>
#include <sstream>
#include <deque>
#include <memory>
#include <random>

using namespace std;
using position = pair<int, int>;
using limits = position;

class World;
class Player;
class Helper;
class Enemy;

#define debug(x) std::cout << #x << " = " << x << "\n";
#define vdebug(a) std::cout << #a << " = "; for(auto x: a) std::cout << x << " "; std::cout << "\n";

class Helper 
{

public:
    std::vector<std::string> splitString(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

};

class Entity {

private:

    int health_;
    int attack_;
    int defense_;
    string name_;
    position entityPosition_;

    deque<position> lastPositions_;

    char sprite_;

    int evaluateDamage(int damage)
    {
        return health_ - damage;
    }

    void push_move(position pos)
    {
        lastPositions_.push_front(pos);
        if(lastPositions_.size() > 3) lastPositions_.pop_back();
    }

public:

    enum class ENTYPE {
        ENEMY,
        PLAYER
    };

    ENTYPE entityType_;

    Entity(string Name, int Health, int Attack, int Defense, position Position,
            char Sprite) :
        name_{Name},
        health_{Health},
        attack_{Attack}, 
        defense_{Defense},
        entityPosition_ {Position},
        sprite_ {Sprite} {}

    deque<position> getLastPositions() { return lastPositions_; }

    void setPosition(position Position, limits worldLimits) 
    {
        position res;

        if(Position.first >= worldLimits.first)        
            res.first = worldLimits.first - 1;
        else if(Position.first <= 0)
            res.first = 0;
        else
            res.first = Position.first;

        if(Position.second >= worldLimits.second)        
            res.second = worldLimits.second - 1;
        else if(Position.second <= 0)
            res.second = 0;
        else
            res.second = Position.second;

        entityPosition_ = res; 
        push_move(res);
    } 

    void setHealth(int health)
    {
        health_ = health;
    }

    void receiveDamage(int damage, World &world);

    string getName() { return name_; }
    position getPosition() { return entityPosition_; }
    char getSprite() { return sprite_; }
    int getHealth() { return health_; }
    int getAttack() { return attack_; }
    int getDefense() { return defense_; }

    virtual ~Entity() = default;
};

using attacks = vector< pair<char, position> >;

class Player : public Entity {

private:
    attacks attackPositions_;
    shared_ptr <Enemy> lastAttackedEnemy;

public:
    Player(string Name, int Health, int Attack, int Defense, position Position,
            char Sprite)
    : Entity(Name, Health, Attack, Defense, Position, Sprite) {
        entityType_ = ENTYPE::PLAYER;
    }

    void moveBackOnePosition(World & world);

    void drawStatus()
    {
        cout << endl;
        cout << this->getName() + "'s  Status \n"; 
        cout << "    Health  : " + to_string( this->getHealth()) << endl;
        cout << "    Attack  : " + to_string( this->getAttack()) << endl;
        cout << "    Defense : " + to_string( this->getDefense()) << endl;
    };

    void attack(string direction, World & world);

    void pushAttack(char attack, position pos)
    {
        attackPositions_.push_back( pair(attack, pos) );
    } 


    void checkCollisions(World& world);

    auto getLastAttackedEnemy()
    {
        return lastAttackedEnemy;
    }

    void setLastAttackedEnemy(shared_ptr <Enemy> lastAttacked)
    {
        lastAttackedEnemy = lastAttacked;
    }

    void clearAttack()
    {
        attackPositions_.clear();
    }

    attacks getAttackPositions()
    {
        return attackPositions_;
    }

};

class Enemy : public Entity {
    
public:

    enum class ENEMY_TYPE {
        BLIND_BAT,
    };

    ENEMY_TYPE enemyType_;

    Enemy(string Name, int Health, int Attack, int Defense, position Position,
            char Sprite)
    : Entity(Name, Health, Attack, Defense, Position, Sprite) {
        entityType_ = ENTYPE::ENEMY;
    }

    ENEMY_TYPE getType()
    {
        return enemyType_;
    }

    virtual void f() {};
};

// Blind bat is an enemy that randomly moves
// around the starting square middle position.
class BlindBat : public Enemy {
    
private: 

    position squareMiddle_;
    vector<position> possiblePositions_;

public:

    BlindBat(string Name, int Health, int Attack, int Defense, position Position,
            char Sprite)
    : Enemy(Name, Health, Attack, Defense, Position, Sprite),
      squareMiddle_(Position)
    {
        enemyType_ = ENEMY_TYPE::BLIND_BAT;

    }

    vector<position> getBatAttackRadiusPositions()
    {
        auto enemyPosition = getPosition();
        possiblePositions_.clear();
        possiblePositions_.push_back( position(enemyPosition.first - 1, enemyPosition.second) );
        possiblePositions_.push_back( position(enemyPosition.first - 1, enemyPosition.second - 1) );
        possiblePositions_.push_back( position(enemyPosition.first + 1, enemyPosition.second + 1) );
        possiblePositions_.push_back( position(enemyPosition.first + 1, enemyPosition.second) );
        possiblePositions_.push_back( position(enemyPosition.first, enemyPosition.second + 1) );
        possiblePositions_.push_back( position(enemyPosition.first - 1, enemyPosition.second + 1) );
        possiblePositions_.push_back( position(enemyPosition.first + 1, enemyPosition.second - 1) );
        possiblePositions_.push_back( position(enemyPosition.first, enemyPosition.second - 1) );
        return possiblePositions_;
    }

    position getSquareMiddle()
    {
        return squareMiddle_;
    }
};

using Scene         = std::vector< std::string >;
using Entities      = unordered_map< Entity::ENTYPE, list< shared_ptr<Entity> > >;
using EntitiesNames = unordered_map< string, shared_ptr<Entity> >;


class World 
{

private:

   Scene worldMap_; 
   Scene baseScene_;

   string worldMessage;
   string debugMessage;

   random_device rd_;
   default_random_engine e_{ rd_() };
   uniform_int_distribution<int> d_{-1, 1};

   const limits worldLimits_ = pair(static_cast<int>(WORLD_CONSTANTS::WORLD_HEIGHT),
                  static_cast<int>(WORLD_CONSTANTS::WORLD_WIDTH));

   bool ShouldDrawEntities_;

   Entities entities_;

   struct Collider
   {
       position pos_;
       char sprite_;
   };

   list<Collider> colliders_;

   enum class WORLD_CONSTANTS : size_t {
       WORLD_HEIGHT = 10,
       WORLD_WIDTH = 60,
   };

   void setupBaseScene()
   {
       addSpriteCollider(baseScene_, 'D', position(5, 5)); 
       addSpriteCollider(baseScene_, 'W', position(5, 4)); 
       addSpriteCollider(baseScene_, 'W', position(6, 4)); 
       addSpriteCollider(baseScene_, 'W', position(7, 4)); 
       addSpriteCollider(baseScene_, 'W', position(7, 5)); 
       addSpriteCollider(baseScene_, 'W', position(7, 6)); 
       addSpriteCollider(baseScene_, 'W', position(6, 6)); 
       addSpriteCollider(baseScene_, 'W', position(5, 6)); 
   }

public:

   World() : baseScene_ {Scene( static_cast<int>(WORLD_CONSTANTS::WORLD_HEIGHT),
               std::string( static_cast<int>(WORLD_CONSTANTS::WORLD_WIDTH), '.' ) )}
   { 
       setupBaseScene();
       worldMap_ = baseScene_; 
   }

   auto getColliders(){ return colliders_; }

   void addSpriteCollider(Scene& Base, 
           char Sprite, position Pos)
   {
       setSprite(Base, Sprite, Pos);
       Collider coll = { Pos, Sprite };
       colliders_.push_back(coll);
   }

   // For now only enemies can be released 
   // i need to think of a better data structure
   // to use instead of a map holding a list.
   void removeEntity(Entity entity)
   {
       auto entityList = &entities_.find(entity.entityType_)->second;

       for(auto it=entityList->begin(); it != entityList->end(); it++)
       {
           if( (*it)->getName() == entity.getName() )
           {
               entityList->erase(it);
               break;
           }
       }
   }

   Scene getWorldMap()
   {
       return worldMap_;
   }

   void setSprite(Scene &scn, char sprite, position pos)
   {
       if(pos.first >= worldLimits_.first)
           return;

       if(pos.first < 0)
           return;

       if(pos.second >= worldLimits_.second)
           return;

       if(pos.second < 0)
           return;

       scn[pos.first][pos.second] = sprite; 
   }

   void resetWorldMap()
   {
       worldMap_ = baseScene_; 
   }

   void drawMap()
   {

       if(ShouldDrawEntities_)
       { 

           // O(n^2) probable bottleneck
           for(auto entities : entities_)
           {
                for(auto entityPointer : entities.second)
                {
                    if(entityPointer)
                    {
                       position entityPosition = entityPointer->getPosition();
                       setSprite(worldMap_, entityPointer->getSprite(), entityPosition);
                    }
                }
           }

       }

       for(auto l : worldMap_)
           cout << l << endl;
   }

   void drawPlayerActions()
   {
       auto players = getPlayers(); 
       for(auto playerEntity : players)
       {
            setDebugMessage(playerEntity->getName());
            auto player = static_pointer_cast<Player>(playerEntity);
            attacks at = player->getAttackPositions();

            for(auto a : at)
            {
                setDebugMessage(to_string(a.first));
                setSprite(worldMap_, a.first, a.second);
            }
       }
   }

   void drawWorldInformation()
   {
       auto player = getPlayer(0); 
       auto last = player->getLastAttackedEnemy(); 
       if(last)
       {
          cout << "\tEnemy health : " << last->getHealth() << endl;
       }
   }

   list< shared_ptr<Entity> > getPlayers()
   {
      auto fo = entities_.find(Entity::ENTYPE::PLAYER); 
      return (*fo).second;
   }

   limits getWorldLimits() const
   {
       return worldLimits_;
   }

   void setShouldDrawEntities(bool value)
   {   
        ShouldDrawEntities_ = value;
   }

   void addEntity(Entity& entity)
   {
      position entityPosition = entity.getPosition();
      position newPosition = entityPosition;

      entity.setPosition(newPosition, worldLimits_);

      if(entity.entityType_ == Entity::ENTYPE::PLAYER)
      {
          entities_[entity.entityType_].push_back( make_shared<Player>(
                      static_cast<Player &>(entity) ) ); 
      }else if(entity.entityType_ == Entity::ENTYPE::ENEMY)
      {
          auto enemy = make_shared<Enemy>( static_cast<Enemy &>(entity) );
          if(enemy->getType() == Enemy::ENEMY_TYPE::BLIND_BAT)
          {
              entities_[entity.entityType_].push_back( make_shared<BlindBat>(
                          static_cast<BlindBat &>(entity) ) ); 
          }else 
          {
              entities_[entity.entityType_].push_back( make_shared<Enemy>(
                          static_cast<Enemy &>(entity) ) ); 
          }
      }
   }

   void EnemiesTurn()
   {
       auto enemies = getEnemies(); 
       for(auto enemyEntity : enemies)
       {

           // Add some ml algorithm to follow the player 
           // or some other thing...
           string enemyName = enemyEntity->getName();
           position oldPosition = enemyEntity->getPosition();
           auto player = getPlayer(0);

           auto enemy = static_pointer_cast<Enemy>(enemyEntity);
           if(enemy->getType() == Enemy::ENEMY_TYPE::BLIND_BAT)
           {
               auto blindbat = static_pointer_cast<BlindBat>(enemy);

               position playerPosition = player->getPosition();
               position batPosition    = blindbat->getPosition(); 
               position squareMiddle   = blindbat->getSquareMiddle();

               if(abs(playerPosition.first - batPosition.first) < 2 
                       && abs(playerPosition.second - batPosition.second) < 2)
               {
                       player->receiveDamage(blindbat->getAttack(), *this);
                       auto batPositions = blindbat->getBatAttackRadiusPositions();
                       for(auto position : batPositions)
                       {
                           setSprite(worldMap_, '^', position);
                       }
               }else
               {
                   enemy->setPosition(pair(squareMiddle.first + d_(e_), 
                               squareMiddle.second + d_(e_)), worldLimits_);  
               }
           }
       }
   }

   // Returns a shared_ptr<Entity> instead
   // of a shared_ptr<Enemy>, otherwise i would have
   // to go through the entire list and cast every
   // class object individually.
   list< shared_ptr<Entity> >& getEnemies()
   {
      auto fo = entities_.find(Entity::ENTYPE::ENEMY); 
      return (*fo).second;
   }

   shared_ptr<Player> getPlayer(size_t playerIndex) const
   {
      auto fo = entities_.find(Entity::ENTYPE::PLAYER);
      auto player = (*fo).second.begin();
      advance(player, playerIndex);
      return static_pointer_cast< Player >(*player);
   }

   void clearTerminal()
   {
       cout << "\x1B[2J\x1B[H";
   }

   void drawMessages()
   {
        cout << endl;
        cout << "\tWorld message : " << worldMessage + '\n';
        cout << "\tWorld debug : "  << debugMessage + '\n';
        cout << endl;
   }

   void setDebugMessage(string message) 
   {
        debugMessage = message;
   }

   void clearPlayerAttacks()
   {
        for(auto playerEntity : getPlayers())
        {
            auto player = static_pointer_cast<Player>(playerEntity);
            player->clearAttack();
        }
   }
    
};

class Parser {

private:
    Helper helper{};

public:
    Parser() = default;

    string trim(string str)
    {
        auto iter = str.begin(); 
        while(*iter++ == ' ');
        auto revIter = str.rbegin(); 
        while(*revIter++ == ' ');
        return string(iter - 1, revIter.base() + 1);
    }

    void parseCommand(string input, World& world)
    {
        string tInput = trim(input);
        auto player = world.getPlayer(0);

        world.clearPlayerAttacks();

        auto tokens = helper.splitString(tInput, ' ');

        position oldPosition = player->getPosition();
        position worldLimits = world.getWorldLimits();

        if(tokens.size() > 0)
        {
           if(tInput == "quit")
           {
               exit(0);
           }else if(tokens[0] == "left" || tokens[0] == "l")
           {
              // Height first [ Because scene is a vector< string > ]
              // then width 
               player->setPosition(pair(oldPosition.first, 
                           oldPosition.second - 1), worldLimits);
           }
           else if(tokens[0] == "right" || tokens[0] == "r")
           {
               player->setPosition(pair(oldPosition.first, 
                           oldPosition.second + 1), worldLimits);
           }
           else if(tokens[0] == "down" || tokens[0] == "d")
           {
               player->setPosition(pair(oldPosition.first + 1,
                           oldPosition.second), worldLimits);
           }
           else if(tokens[0] == "up" || tokens[0] == "u")
           {
               player->setPosition(pair(oldPosition.first - 1, 
                           oldPosition.second), worldLimits);
           }else if(tokens[0] == "back" || tokens[0] == "b")
           {
               player->moveBackOnePosition(world);
           }
        }

        if(tokens.size() > 1)
        {
            if(tokens[0] == "attack" && tokens[1] == "left" 
               || tokens[0] == "a" && tokens[1] == "l")
            {
                player->attack("left", world);
            }
            else if(tokens[0] == "attack" && tokens[1] == "right" 
               || tokens[0] == "a" && tokens[1] == "r")
            {
                player->attack("right", world);
            }
            else if(tokens[0] == "attack" && tokens[1] == "down" 
               || tokens[0] == "a" && tokens[1] == "d")
            {
                player->attack("down", world);
            }
            else if(tokens[0] == "attack" && tokens[1] == "up" 
               || tokens[0] == "a" && tokens[1] == "u")
            {
                player->attack("up", world);
            }
        }
    }

};

void Player::attack(string direction, World & world)
{
    auto playerPosition = getPosition();
    auto worldMap_ = world.getWorldMap();
    auto enemies = world.getEnemies();

    clearAttack();

    if(direction == "up")
    {
        auto pos = pair(playerPosition.first - 1,
                playerPosition.second);

        for(auto entity : enemies)
        {
            auto enemy = static_pointer_cast<Enemy>(entity);
            if(enemy->getPosition() == pos)
            {
                setLastAttackedEnemy(enemy);
                enemy->receiveDamage(getAttack(), world);
            }
        }

        pushAttack('|', pos);
        pos.first -= 1;

        for(auto entity : enemies)
        {
            auto enemy = static_pointer_cast<Enemy>(entity);
            if(enemy->getPosition() == pos)
            {
                setLastAttackedEnemy(enemy);
                enemy->receiveDamage(getAttack(), world);
            }
        }

        pushAttack('|', pos);
        
    }else if(direction == "left")
    {
        auto pos = pair(playerPosition.first,
                playerPosition.second - 1);
         
        for(auto entity : enemies)
        {
            auto enemy = static_pointer_cast<Enemy>(entity);
            if(enemy->getPosition() == pos)
            {
                setLastAttackedEnemy(enemy);
                enemy->receiveDamage(getAttack(), world);
            }
        }

        pushAttack('\\', pos);
        pos.second -= 1;

        for(auto entity : enemies)
        {
            auto enemy = static_pointer_cast<Enemy>(entity);
            if(enemy->getPosition() == pos)
            {
                setLastAttackedEnemy(enemy);
                enemy->receiveDamage(getAttack(), world);
            }
        }

        pushAttack('\\', pos);
    }else if(direction == "right")
    {
        auto pos = pair(playerPosition.first,
                playerPosition.second + 1);

        for(auto entity : enemies)
        {
            auto enemy = static_pointer_cast<Enemy>(entity);
            if(enemy->getPosition() == pos)
            {
                setLastAttackedEnemy(enemy);
                enemy->receiveDamage(getAttack(), world);
            }
        }

        pushAttack('/', pos);
        pos.second += 1;

        for(auto entity : enemies)
        {
            auto enemy = static_pointer_cast<Enemy>(entity);
            if(enemy->getPosition() == pos)
            {
                setLastAttackedEnemy(enemy);
                enemy->receiveDamage(getAttack(), world);
            }
        }

        pushAttack('/', pos);
    }else if(direction == "down")
    {
        auto pos = pair(playerPosition.first + 1,
                playerPosition.second);

        for(auto entity : enemies)
        {
            auto enemy = static_pointer_cast<Enemy>(entity);
            if(enemy->getPosition() == pos)
            {
                setLastAttackedEnemy(enemy);
                enemy->receiveDamage(getAttack(), world);
            }
        }

        pushAttack('|', pos);

        pos.first += 1;

        for(auto entity : enemies)
        {
            auto enemy = static_pointer_cast<Enemy>(entity);
            if(enemy->getPosition() == pos)
            {
                setLastAttackedEnemy(enemy);
                enemy->receiveDamage(getAttack(), world);
            }
        }

        pushAttack('|', pos);
    }
}

void Entity::receiveDamage(int damage, World &world)
{
    int newHealth = evaluateDamage(damage);      
    if(newHealth > 0)
    {
        this->setHealth(newHealth);
    } 
    else
    {
        world.removeEntity(*this);
    }
}

void Player::checkCollisions(World& world)
{
    auto enemies = world.getEnemies();
    auto colliders = world.getColliders();

    for(auto enemy : enemies)
    {
        if(enemy->getPosition() == this->getPosition())
        {
            moveBackOnePosition(world);
        }
    }

    for(auto collider : colliders)
    {
        if(collider.pos_ == this->getPosition())
        {
            moveBackOnePosition(world);
        }
    }
}

void Player::moveBackOnePosition(World & world)
{
    auto last_ = getLastPositions();
    if(last_.size() > 1)
    {
        setPosition(last_[1], world.getWorldLimits());
    }
}

int main(void)
{
    World world{};
    Parser parser{};

    Player player1("John", 200, 20, 30,  position(4, 23), 'J');
    BlindBat bat1("Blind Bat 1", 30, 5, 1, position(5, 9), 'B');
    BlindBat bat2("Blind Bat 2", 30, 5, 1, position(5, 34), 'B');
    BlindBat bat3("Blind Bat 3", 30, 5, 1, position(5, 59), 'B');

    string temp;
    cout << "Game starting... Type anything to continue\n" << endl;
    cin  >> temp;

    world.setShouldDrawEntities(true);

    world.addEntity(player1);
    world.addEntity(bat1);
    world.addEntity(bat2);
    world.addEntity(bat3);

    auto player = world.getPlayer(0);

    for(;;)
    {
        string input;

        world.clearTerminal();
        world.resetWorldMap();

        player->checkCollisions(world);
        player->drawStatus();

        world.EnemiesTurn(); 
        world.drawPlayerActions();

        world.drawMap();
        world.drawMessages();
        world.drawWorldInformation();
        getline(cin, input);

        // Players turn
        parser.parseCommand(input, world);
    }

    return EXIT_SUCCESS;
}

