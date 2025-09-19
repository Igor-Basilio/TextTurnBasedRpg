
#include <iostream>
#include <vector>
#include <unordered_map>
#include <list>
#include <assert.h>
#include <memory>
#include <random>

using namespace std;
using position = pair<int, int>;
using limits = position;

class Entity {

private:
    int health_;
    int attack_;
    int defense_;
    string name_;
    position entityPosition_;
    char sprite_;

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
    } 

    void setHealth(int health)
    {
        health_ = health;
    }

    string getName() { return name_; }
    position getPosition() { return entityPosition_; }
    char getSprite() { return sprite_; }
    int getHealth() { return health_; }
    int getAttack() { return attack_; }
    int getDefense() { return defense_; }

    virtual ~Entity() {}
};

class Player : public Entity {

public:
    Player(string Name, int Health, int Attack, int Defense, position Position,
            char Sprite)
    : Entity(Name, Health, Attack, Defense, Position, Sprite) {
        entityType_ = ENTYPE::PLAYER;
    }

    void receiveDamage(int damage)
    {
        this->setHealth( this->getHealth() - damage );
    }

    void drawStatus()
    {
        cout << endl;
        cout << this->getName() + "'s  Status \n"; 
        cout << "    Health  : " + to_string( this->getHealth()) << endl;
        cout << "    Attack  : " + to_string( this->getAttack()) << endl;
        cout << "    Defense : " + to_string( this->getDefense()) << endl;
    };
    
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
    virtual ~Enemy() = default;
};

// Blind bat is an enemy that randomly moves
// around the starting square middle position.
class BlindBat : public Enemy {
    
private: 

    position squareMiddle_;
    vector<position> possiblePositions_;

    // Chat gpt generated
    static std::vector<position> square_neighborhood(position c, int r = 1) {
        std::vector<position> out;
        out.reserve((2*r + 1) * (2*r + 1));
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx)
                out.emplace_back(c.first + dx, c.second + dy);
        return out;
    }

public:

    BlindBat(string Name, int Health, int Attack, int Defense, position Position,
            char Sprite)
    : Enemy(Name, Health, Attack, Defense, Position, Sprite),
      squareMiddle_(Position)
    {
        enemyType_ = ENEMY_TYPE::BLIND_BAT;
        // possiblePositions_.push_back( position(squareMiddle_.first - 1, squareMiddle_.second) );

        possiblePositions_ = square_neighborhood(squareMiddle_, 2);
    }

    vector<position> getPossiblePositions()
    {
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

   random_device rd_;
   default_random_engine e_{ rd_() };
   uniform_int_distribution<int> d_{-1, 1};

   const limits worldLimits_ = pair(static_cast<int>(WORLD_CONSTANTS::WORLD_HEIGHT),
                  static_cast<int>(WORLD_CONSTANTS::WORLD_WIDTH));

   bool ShouldDrawEntities_;

   Entities entities_;

   enum class WORLD_CONSTANTS : size_t {
       WORLD_HEIGHT = 10,
       WORLD_WIDTH = 60,
   };

public:

   World() : baseScene_ {Scene( static_cast<int>(WORLD_CONSTANTS::WORLD_HEIGHT),
               std::string( static_cast<int>(WORLD_CONSTANTS::WORLD_WIDTH), '.' ) )}
   { 
       worldMap_ = baseScene_; 
   }

   void resetWorldMap()
   {
       worldMap_ = baseScene_; 
   }

   void drawMap()
   {

       if(ShouldDrawEntities_)
       { 

           // O(n^2)
           for(auto entities : entities_)
           {
                for(auto entityPointer : entities.second)
                {
                   position entityPosition = entityPointer->getPosition();
                   worldMap_[entityPosition.first][entityPosition.second] =
                       entityPointer->getSprite(); 
                }
           }

       }

       for(auto l : worldMap_)
           cout << l << endl;
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

               if(abs(playerPosition.first - batPosition.first) <= 3 
                       && abs(playerPosition.second - batPosition.second) <= 3)
               {
                       player->receiveDamage(blindbat->getAttack());
                       auto batPositions = blindbat->getPossiblePositions();
                       for(auto position : batPositions)
                       {
                           worldMap_[position.first][position.second] = '^';
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
   list< shared_ptr<Entity> > getEnemies()
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
    
};

class Parser {

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

    void parseCommand(string input, const World& world)
    {
        string tInput = trim(input);
        auto player = world.getPlayer(0);

        position oldPosition = player->getPosition();
        position worldLimits = world.getWorldLimits();

        if(tInput == "quit")
        {
            exit(0);
        }else if(tInput == "left")
        {
            // Height first [ Because scene is a vector< string > ]
            // then width 
            player->setPosition(pair(oldPosition.first, oldPosition.second - 1), worldLimits);
        }
        else if(tInput == "right")
        {
            player->setPosition(pair(oldPosition.first, oldPosition.second + 1), worldLimits);
        }
        else if(tInput == "down")
        {
            player->setPosition(pair(oldPosition.first + 1, oldPosition.second), worldLimits);
        }
        else if(tInput == "up")
        {
            player->setPosition(pair(oldPosition.first - 1, oldPosition.second), worldLimits);
        }
    }

};

int main(void)
{
    World world{};
    Parser parser{};

    Player player1("John", 200, 20, 30,  position(4, 23), 'J');
    BlindBat bat1("Blind Bat", 30, 5, 1, position(5, 9), 'B');
    BlindBat bat2("Blind Bat", 30, 5, 1, position(5, 34), 'B');

    string temp;
    cout << "Game starting... Type anything to continue\n" << endl;
    cin  >> temp;

    world.setShouldDrawEntities(true);

    world.addEntity(player1);
    world.addEntity(bat1);
    world.addEntity(bat2);

    auto player = world.getPlayer(0);

    for(;;)
    {
        string input;

        world.clearTerminal();
        world.resetWorldMap();
        player->drawStatus();
        world.EnemiesTurn(); 
        world.drawMap();

        cin >> input;

        // Players turn
        parser.parseCommand(input, world);
    }

    return EXIT_SUCCESS;
}

