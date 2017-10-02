#include "MainScene.h"
#include <time.h>
#include "File.h"
#include "TextureManager.h"
#include "SubScene.h"
#include "Save.h"
#include "UI.h"

MainScene MainScene::main_map_;

MainScene::MainScene()
{
    full_window_ = 1;

    if (!data_readed_)
    {
        earth_layer_.resize(COORD_COUNT);
        surface_layer_.resize(COORD_COUNT);
        building_layer_.resize(COORD_COUNT);
        build_x_layer_.resize(COORD_COUNT);
        build_y_layer_.resize(COORD_COUNT);

        int length = COORD_COUNT * COORD_COUNT * sizeof(SAVE_INT);

        File::readFile("../game/resource/earth.002", &earth_layer_(0), length);
        File::readFile("../game/resource/surface.002", &surface_layer_(0), length);
        File::readFile("../game/resource/building.002", &building_layer_(0), length);
        File::readFile("../game/resource/buildx.002", &build_x_layer_(0), length);
        File::readFile("../game/resource/buildy.002", &build_y_layer_(0), length);

        divide2(earth_layer_);
        divide2(surface_layer_);
        divide2(building_layer_);
    }
    data_readed_ = true;

    //100����
    for (int i = 0; i < 100; i++)
    {
        auto c = new Cloud();
        cloud_vector_.push_back(c);
        c->initRand();
    }
    //getEntrance();
}

MainScene::~MainScene()
{
    for (int i = 0; i < cloud_vector_.size(); i++)
    {
        delete cloud_vector_[i];
    }
}

void MainScene::divide2(MapSquare& m)
{
    for (int i = 0; i < m.size(); i++)
    {
        m(i) /= 2;
    }
}

void MainScene::draw()
{
    Engine::getInstance()->setRenderAssistTexture();
    //LOG("main\n");
    int k = 0;
    auto t0 = Engine::getInstance()->getTicks();
    struct DrawInfo { int i; Point p; };
    std::map<int, DrawInfo> map;
    //TextureManager::getInstance()->renderTexture("mmap", 0, 0, 0);
#ifdef _DEBUG
    Engine::getInstance()->fillColor({ 0, 0, 0, 255 }, 0, 0, -1, -1);
#endif
    for (int sum = -view_sum_region_; sum <= view_sum_region_ + 15; sum++)
    {
        for (int i = -view_width_region_; i <= view_width_region_; i++)
        {
            int i1 = man_x_ + i + (sum / 2);
            int i2 = man_y_ - i + (sum - sum / 2);
            auto p = getPositionOnScreen(i1, i2, man_x_, man_y_);
            //auto p = getMapPoint(i1, i2, *_Mx, *_My);
            if (i1 >= 0 && i1 < COORD_COUNT && i2 >= 0 && i2 < COORD_COUNT)
            {
                //����3�㣬���棬���棬���������ǰ����ڽ�����
#ifndef _DEBUG
                //����ģʽ�²��������棬ͼ������̫��ռ��CPU�ܴ�
                if (earth_layer_(i1, i2) > 0)
                {
                    TextureManager::getInstance()->renderTexture("mmap", earth_layer_(i1, i2), p.x, p.y);
                }
#endif
                if (surface_layer_(i1, i2) > 0)
                {
                    TextureManager::getInstance()->renderTexture("mmap", surface_layer_(i1, i2), p.x, p.y);
                }
                if (building_layer_(i1, i2) > 0)
                {
                    auto t = building_layer_(i1, i2);
                    //����ͼƬ�Ŀ��ȼ���ͼ���е�, Ϊ�������С��, ʵ�����е������2��
                    //��Ҫ����������y����
                    //ֱ������z��
                    auto tex = TextureManager::getInstance()->loadTexture("mmap", t);
                    auto w = tex->w;
                    auto h = tex->h;
                    auto dy = tex->dy;
                    int c = ((i1 + i2) - (w + 35) / 36 - (dy - h + 1) / 9) * 1024 + i1;
                    map[2 * c + 1] = { t, p };
                }
                if (i1 == man_x_ && i2 == man_y_)
                {
                    if (isWater(man_x_, man_y_))
                    {
                        man_pic_ = SHIP_PIC_0 + Scene::towards_ * SHIP_PIC_COUNT + step_;
                    }
                    else
                    {
                        man_pic_ = MAN_PIC_0 + Scene::towards_ * MAN_PIC_COUNT + step_;  //ÿ������ĵ�һ���Ǿ�ֹͼ
                        if (rest_time_ >= BEGIN_REST_TIME)
                        {
                            man_pic_ = REST_PIC_0 + Scene::towards_ * REST_PIC_COUNT + (rest_time_ - BEGIN_REST_TIME) / REST_INTERVAL % REST_PIC_COUNT;
                        }
                    }
                    int c = 1024 * (i1 + i2) + i1;
                    map[2 * c] = { man_pic_, p };
                }
            }
            k++;
        }
    }
    for (auto i = map.begin(); i != map.end(); i++)
    {
        TextureManager::getInstance()->renderTexture("mmap", i->second.i, i->second.p.x, i->second.p.y);
    }
    auto t1 = Engine::getInstance()->getTicks();
    //�Ƶ���ͼ
    for (auto& c : cloud_vector_)
    {
        //c->draw();
        c->setPositionOnScreen(man_x_, man_y_, screen_center_x_, screen_center_y_);
    }
    //log("%d\n", t1 - t0);
    Engine::getInstance()->renderAssistTextureToWindow();
}

//��ʱ��������ͼ�Լ�һЩ��������
void MainScene::dealEvent(BP_Event& e)
{
    int x = man_x_, y = man_y_;
    //���ܼ�
    if (e.type == BP_KEYUP && e.key.keysym.sym == BPK_ESCAPE
        || e.type == BP_MOUSEBUTTONUP && e.button.button == BP_BUTTON_RIGHT)
    {
        UI::getInstance()->run();
        clearEvent(e);
    }

    //������·���֣����4�������
    int pressed = 0;
    for (auto i = int(BPK_RIGHT); i <= int(BPK_UP); i++)
    {
        if (i != pre_pressed_ && Engine::getInstance()->checkKeyPress(i))
        {
            pressed = i;
        }
    }
    if (pressed == 0 && Engine::getInstance()->checkKeyPress(pre_pressed_))
    {
        pressed = pre_pressed_;
    }
    pre_pressed_ = pressed;

    if (pressed)
    {
        getTowardsFromKey(pressed);
        getTowardsPosition(man_x_, man_y_, towards_, &x, &y);
        tryWalk(x, y, towards_);
        if (total_step_ <= 1)
        {
            Engine::getInstance()->delay(50);
        }
        total_step_++;
    }
    else
    {
        total_step_ = 0;
    }

    if (pressed && checkEntrance(x, y))
    {
        clearEvent(e);
        total_step_ = 0;
    }

    rest_time_++;

    //���Ѱ·��δ���
    if (e.type == BP_MOUSEBUTTONUP && e.button.button == BP_BUTTON_LEFT)
    {
        //getMousePosition(e.button.x, e.button.y);
        //stopFindWay();
        //if (canWalk(mouse_x_, mouse_y_) && !isOutScreen(mouse_x_, mouse_y_))
        //{
        //    FindWay(man_x_, man_y_, mouse_x_, mouse_y_);
        //}
    }
}

void MainScene::entrance()
{
    calViewRegion();
    man_x_ = Save::getInstance()->MainMapX;
    man_y_ = Save::getInstance()->MainMapY;
}

void MainScene::exit()
{
}

void MainScene::tryWalk(int x, int y, Towards t)
{
    if (canWalk(x, y))
    {
        man_x_ = x;
        man_y_ = y;
    }
    step_++;
    if (isWater(man_x_, man_y_))
    {
        step_ = step_ % SHIP_PIC_COUNT;
    }
    else
    {
        if (step_ >= MAN_PIC_COUNT)
        {
            step_ = 1;
        }
    }
    rest_time_ = 0;
}

bool MainScene::isBuilding(int x, int y)
{

    if (building_layer_(build_x_layer_(x, y), build_y_layer_(x, y)) > 0)
    {
        return  true;
    }
    else
    {
        return false;
    }
}

bool MainScene::isWater(int x, int y)
{
    auto pic = earth_layer_(x, y);
    if (pic == 419 || pic >= 306 && pic <= 335)
    {
        return true;
    }
    else if (pic >= 179 && pic <= 181
        || pic >= 253 && pic <= 335
        || pic >= 508 && pic <= 511)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool MainScene::isOutLine(int x, int y)
{
    return (x < 0 || x > COORD_COUNT || y < 0 || y > COORD_COUNT);
}

bool MainScene::canWalk(int x, int y)
{
    if (isBuilding(x, y) || isOutLine(x, y)/*|| checkIsWater(x, y)*/)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool MainScene::checkEntrance(int x, int y)
{
    for (int i = 0; i < Save::getInstance()->submap_infos_.size(); i++)
    {
        auto s = Save::getInstance()->getSubMapInfo(i);
        if (x == s->MainEntranceX1 && y == s->MainEntranceY1 || x == s->MainEntranceX2 && y == s->MainEntranceY2)
        {
            bool can_enter = false;
            if (s->EntranceCondition == 0)
            {
                can_enter = true;
            }
            else if (s->EntranceCondition == 2)
            {
                //ע���������2���趨
                for (auto r : Save::getInstance()->Team)
                {
                    if (Save::getInstance()->getRole(r)->Speed >= 70)
                    {
                        can_enter = true;
                        break;
                    }
                }
            }
            if (can_enter)
            {
                auto sub_map = new SubScene(i);
                sub_map->run();
                return true;
            }
        }
    }
    //if (Entrance[x][y] > 0 && Entrance[x][y] <= config::MAXScene)
    //{
    //    Save::getInstance()->m_BasicData[0].m_sMx = Mx;
    //    Save::getInstance()->m_BasicData[0].m_sMx = My;
    //    Save::getInstance()->m_BasicData[0].m_sMFace = towards;
    //    Save::getInstance()->m_BasicData[0].m_sWhere = 1;
    //    auto s = new SubScene(Entrance[x][y]);
    //    push(s);
    //    return true;
    //}
    return false;
}

void MainScene::getEntrance()
{
    //for (int x = 0; x < maxX; x++)
    //    for (int y = 0; y < maxY; y++)
    //    { Entrance[x][y] = -1; }
    //for (int i = 0; i < Save::getInstance()->m_SceneData.size(); i++)
    //{

    //    int x = Save::getInstance()->m_SceneData[i].MainEntranceX1;
    //    int y = Save::getInstance()->m_SceneData[i].MainEntranceY1;
    //    if (x > 0 && y > 0 && x < maxX && y < maxY)
    //    {
    //        Entrance[x][y] = i;
    //    }
    //    x = Save::getInstance()->m_SceneData[i].MainEntranceX2;
    //    y = Save::getInstance()->m_SceneData[i].MainEntranceY2;
    //    if (x > 0 && y > 0 && x < maxX && y < maxY)
    //    {
    //        Entrance[x][y] = i;
    //    }
    //}
}

bool MainScene::isOutScreen(int x, int y)
{
    return (abs(man_x_ - x) >= 2 * view_width_region_ || abs(man_y_ - y) >= view_sum_region_);
}
