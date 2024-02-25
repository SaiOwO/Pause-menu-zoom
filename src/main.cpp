#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class ZoomLayer : public CCLayer {
public:
    float posX = 0, posY = 0;
    PlayLayer* m_playLayer = PlayLayer::get();
    CCLabelBMFont* m_zoomTxt;

    bool init() {
        if (!CCLayer::init()) return false;

        auto size = CCDirector::sharedDirector()->getWinSize();
        auto running = CCDirector::sharedDirector()->getRunningScene();

        if (running->getChildByID("zoom-label"_spr)) {
            m_zoomTxt = reinterpret_cast<CCLabelBMFont*>(running->getChildByID("zoom-label"_spr));
        }
        else {
            m_zoomTxt = CCLabelBMFont::create("", "bigFont.fnt");
            m_zoomTxt->setPosition({ size.width / 2, size.height / 2 - 100 });
            m_zoomTxt->setScale(.7f);
            m_zoomTxt->setID("zoom-label"_spr);
            running->addChild(m_zoomTxt, running->getHighestChildZ());
        }

        this->setID("zoom-layer"_spr);
        this->setTouchEnabled(true);
        this->scheduleUpdate();

        this->setMouseEnabled(true);
        this->setTouchMode(ccTouchesMode::kCCTouchesOneByOne);
        
        return true;
    }
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) {
        if (m_touches.size() == 1) {
            auto firstTouch = *m_touches.begin();

            auto firstLoc = firstTouch->getLocation();
            auto secondLoc = touch->getLocation();

            m_touchMidPoint = (firstLoc + secondLoc) / 2.f;
            m_initialScale = m_playLayer->getScale();
            m_initialDistance = firstLoc.getDistance(secondLoc);

            m_touches.insert(touch);
            return true;
        }
        else if (CCLayer::ccTouchBegan(touch, event)) {
            m_touches.insert(touch);
            return true;
        }
        
        return false;
    }
    std::unordered_set<Ref<CCTouch>> m_touches;
    float m_initialDistance = 0.f;
    float m_initialScale = 1.f;
    CCPoint m_touchMidPoint;

    void ccTouchMoved(CCTouch* touch, CCEvent* event) {
        if (m_touches.size() == 2) { // better edit
            auto objLayer = m_playLayer;

            auto it = m_touches.begin();
            auto firstTouch = *it;
            ++it;
            auto secondTouch = *it;

            auto firstLoc = firstTouch->getLocation();
            auto secondLoc = secondTouch->getLocation();

            auto center = (firstLoc + secondLoc) / 2.f;
            auto distNow = firstLoc.getDistance(secondLoc);
            
            auto const mult = m_initialDistance / distNow;
            auto zoom = std::max(m_initialScale / mult, 0.1f);

            if (zoom < 1.0f) {
                zoom = 1.0f;
                m_playLayer->setScale(zoom);
            }
            else if (zoom > 10.0f) {
                zoom = 10.0f;
                m_playLayer->setScale(zoom);
            }
            else {
                m_playLayer->setScale(zoom);
            }

            auto centerDiff = m_touchMidPoint - center;
            objLayer->setPosition(objLayer->getPosition() - centerDiff);

            m_touchMidPoint = center;

            if (zoom <= 1.0f) {
                this->getParent()->setVisible(true);
                m_playLayer->setPosition(0,0);
            }
            else {
                this->getParent()->setVisible(false);
            }

            m_zoomTxt->setString(CCString::createWithFormat("Zoom: x%.02f", zoom)->getCString());
            m_zoomTxt->runAction(CCFadeOut::create(0.5));
        }
        else {
            CCPoint pos = touch->getDelta() / m_playLayer->getScale();
            posX += pos.x;
            posY += pos.y;
        }
    }
    void ccTouchEnded(CCTouch* touch, CCEvent* event) {
        m_touches.erase(touch);
    }
    void update(float dt) {
        if (m_playLayer->getScale() > 1.0f) {
            float cantidadX = (m_playLayer->getContentSize().width / 2);
            float cantidadY = (m_playLayer->getContentSize().height / 2);

            posX = clamp(posX, -cantidadX, cantidadX);
            posY = clamp(posY, -cantidadY, cantidadY);
        }
        else {
            posX = 0;
            posY = 0;
        }

        m_playLayer->setPosition(posX * m_playLayer->getScale(), posY * m_playLayer->getScale());
    }
    void updateVisibility() {
        if (m_playLayer->getScale() <= 1.0f) {
            if (this->getParent())
                this->getParent()->setVisible(true);
            m_playLayer->setPosition(0,0);
        }
        else {
            if (this->getParent())
                this->getParent()->setVisible(false);
        }
    }
	void scrollWheel(float y, float x) {
        auto zoom = m_playLayer->getScale() - (y * (0.01f * m_playLayer->getScale()));
        zoom = clamp(zoom, 1.0f, 10.f);
        m_playLayer->setScale(zoom);

        this->updateVisibility();

        m_zoomTxt->setString(CCString::createWithFormat("Zoom: x%.02f", zoom)->getCString());
        m_zoomTxt->stopAllActions();
        m_zoomTxt->setOpacity(255);
        m_zoomTxt->runAction(CCSequence::createWithTwoActions(CCDelayTime::create(0.5f), CCFadeOut::create(0.5)));
    }
    CREATE_FUNC(ZoomLayer)
};

class $modify(PlayLayer) {
    void resetLevel() {
        auto pl = PlayLayer::get();
        pl->setPosition(0,0);
        pl->setScale(1);
        PlayLayer::resetLevel();
    }
};

class $modify(Extra, PauseLayer) {
    void addWithDelay(CCObject* l) {
        this->setPreviousPriority(-501);
        this->setTouchPriority(-501);

        auto z = ZoomLayer::create();
        z->setPreviousPriority(-501);
        z->setTouchPriority(-501);
        reinterpret_cast<CCLayer*>(l)->addChild(z, -1);
    }
    void customSetup() {
        PauseLayer::customSetup();
        this->runAction(CCSequence::createWithTwoActions(CCDelayTime::create(0.1f), CCCallFuncO::create(this, callfuncO_selector(Extra::addWithDelay), this)));
    }

    void onResume(CCObject* p0) {
        auto pl = PlayLayer::get();
        pl->setPosition(0,0);
        pl->setScale(1);
        PauseLayer::onResume(p0);
    }

    void keyDown(enumKeyCodes p0) {
        auto zoomLayer = reinterpret_cast<ZoomLayer*>(this->getChildByID("zoom-layer"_spr));

        switch (p0) { 
            case KEY_Escape:
            case CONTROLLER_B:
                {
                    auto pl = PlayLayer::get();
                    if (pl->getScale() > 1) {
                        pl->setPosition(0,0);
                        pl->setScale(1);
                    }

                    if (this->isVisible()) {
                        PauseLayer::keyDown(p0);
                    }
                    else {
                        if (zoomLayer) {
                            zoomLayer->setTouchEnabled(false);
                            zoomLayer->unscheduleUpdate();
                            zoomLayer->posX = 0;
                            zoomLayer->posY = 0;
                        }
                    }
                    this->setVisible(true);
                }
                break;
            default:
                PauseLayer::keyDown(p0);
                break;
        }
    }
};