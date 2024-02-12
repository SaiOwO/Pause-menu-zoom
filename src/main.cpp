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
        auto size = CCDirector::sharedDirector()->getWinSize();
        auto running = CCDirector::sharedDirector()->getRunningScene();

        if (running->getChildByTag(65)) {
            m_zoomTxt = reinterpret_cast<CCLabelBMFont*>(running->getChildByTag(65));
        }
        else {
            m_zoomTxt = CCLabelBMFont::create("", "bigFont.fnt");
            m_zoomTxt->setPosition({ size.width / 2, size.height / 2 - 100 });
            m_zoomTxt->setScale(.7f);
            m_zoomTxt->setTag(65);
            running->addChild(m_zoomTxt, running->getHighestChildZ());
        }
        this->setTag(64);
        this->setMouseEnabled(true);
        this->setTouchMode(ccTouchesMode::kCCTouchesOneByOne);
        return true;
    }
    void ccTouchMoved(CCTouch* touch, CCEvent* event) {
        CCPoint pos = touch->getDelta() / m_playLayer->getScale();
        posX += pos.x;
        posY += pos.y;
    }
    void update(float dt) {
        if (m_playLayer->getScale() > 1.0f) {
	    // i put "(m_playLayer->getScaledContentSize().width / 2) / m_playLayer->getScale()" for fun xd
            float cantidadX = m_playLayer->getContentSize().width / 2;
            float cantidadY = m_playLayer->getContentSize().height / 2;

            posX = clamp(posX, -cantidadX, cantidadX);
            posY = clamp(posY, -cantidadY, cantidadY);
        }
        else {
            posX = 0;
            posY = 0;
        }

        m_playLayer->setPosition(posX * m_playLayer->getScale(), posY * m_playLayer->getScale());
    }
	void scrollWheel(float y, float x) {
        auto zoom = m_playLayer->getScale() - (y * (0.01f * m_playLayer->getScale()));
        zoom = clamp(zoom, 1.0f, 10.f);
        m_playLayer->setScale(zoom);

        if (zoom <= 1.0f) {
            this->getParent()->setVisible(true);
            this->setTouchEnabled(false);
            this->unscheduleUpdate();
            m_playLayer->setPosition(0,0);
        }
        else {
            this->getParent()->setVisible(false);
            this->setTouchEnabled(true);
            this->scheduleUpdate();
        }

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

class $modify(PauseLayer) {
    static PauseLayer* create(bool p0) {
        auto r = PauseLayer::create(p0);
        r->addChild(ZoomLayer::create());
        return r;
    }

    void onResume(CCObject* p0) {
        auto pl = PlayLayer::get();
        pl->setPosition(0,0);
        pl->setScale(1);
        PauseLayer::onResume(p0);
    }

    void keyDown(enumKeyCodes p0) {
        auto zoomLayer = reinterpret_cast<ZoomLayer*>(this->getChildByTag(64));

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
