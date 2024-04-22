
#include <graph2x.hpp>
#include <axxegro/axxegro.hpp>
#include <list>


al::Vec3f ColorToVec(al::Color color) {
	return al::Vec3f{color.r, color.g, color.b};
}
al::Color VecToColor(al::Vec3f vec) {
	return al::RGB_f(vec.x, vec.y, vec.z);
}

namespace glob {
	al::Font DefaultFont = al::Font::CreateBuiltinFont();
}

void DrawText(const std::string& text, al::Color color, al::Vec2i pos, int align = ALLEGRO_ALIGN_LEFT) {
	glob::DefaultFont.drawText(text, color, pos, align);
}

void DrawShadowedText(const std::string& text, al::Color color, al::Vec2i pos, int align = ALLEGRO_ALIGN_LEFT) {
	al::Color shadowColor = color;
	auto& [r, g, b, a] = shadowColor;
	r /= 4.0; g /= 4.0; b /= 4.0; a = 0.5;
	DrawText(text, shadowColor, pos + al::Vec2i{2, 2}, align);
	DrawText(text, color, pos, align);
}


struct Entity {
	virtual void tick(double time) = 0;
	virtual void render() = 0;
	virtual bool onEvent(const al::Event&) = 0;
	virtual int getZOrder() = 0;
	virtual ~Entity() = default;
};

struct World {
public:

	friend struct EntityHandle;

	template<typename T>
		requires std::is_base_of_v<Entity, T>
	struct EntityHandle {
		World* world;
		std::list<std::unique_ptr<Entity>>::iterator iter;

		[[nodiscard]] T& get() {
			return *static_cast<T*>(*iter);
		}
		[[nodiscard]] const T& get() const {
			return *static_cast<const T*>(*iter);
		}

		~EntityHandle() {
			world->entities.erase(iter);
		}
	};

	template<typename T, typename... Ts>
		requires std::is_base_of_v<Entity, T>
	EntityHandle<T> createEntity(Ts&&... args) {
		return EntityHandle<T> {
			.world = this,
			.iter = entities.insert(entities.end(), std::make_unique<T>(std::forward<Ts...>(args...)))
		};
	}

	void tick(double time) {
		for(const auto& sp: entities) {
			sp->tick(time);
		}
	}

	void render() {
		auto entityPtrs = allEntities();
		std::ranges::sort(entityPtrs, [](Entity* a, Entity* b) {
			return b->getZOrder() > a->getZOrder();
		});
		for(Entity* e: entityPtrs) {
			e->render();
		}
	}

	void onEvent(const al::Event& ev) {
		auto entityPtrs = allEntities();
		std::ranges::sort(entityPtrs, [](Entity* a, Entity* b) {
			return b->getZOrder() > a->getZOrder();
		});
		for(Entity* e: entityPtrs) {
			if(e->onEvent(ev)) {
				break;
			}
		}
	}


private:

	[[nodiscard]] std::vector<Entity*> allEntities() const {
		std::vector<Entity*> result;
		for(const auto& sp: entities) {
			result.push_back(sp.get());
		}
		return result;
	}

	std::list<std::unique_ptr<Entity>> entities;
};

struct Button: public Entity {
public:
	struct Def {
		al::RectI rect;
		std::string caption;
		std::function<void(void)> callback = [](){};
	} def;

	Button(const Def& def): def(def) {

	}


	enum class State {
		Inactive,
		Hover,
		Down
	};


	void tick(double time) override {
		static constexpr double half_life = 1.0 / 15.0;
		double w = std::exp2(-time / half_life);
		currentColor = VecToColor(ColorToVec(currentColor)*w + ColorToVec(targetColor())*(1.0-w));
	}

	void render() override {
		al::TargetBitmap.setClippingRectangle(def.rect);
		al::DrawFilledRectangle(def.rect, currentColor);
		al::Color ccBrightened = VecToColor(ColorToVec(currentColor) + al::Vec3f{0.15, 0.15, 0.15});
		al::Color ccDarkened = VecToColor(ColorToVec(currentColor) - al::Vec3f{0.15, 0.15, 0.15});

		al::DrawLine(def.rect.bottomLeft().asFloat(), def.rect.topLeft().asFloat(), state!=State::Down ? ccBrightened : ccDarkened, 4.0);
		al::DrawLine(def.rect.topLeft().asFloat(), def.rect.topRight().asFloat(), state!=State::Down ? ccBrightened : ccDarkened, 4.0);
		al::DrawLine(def.rect.bottomLeft().asFloat(), def.rect.bottomRight().asFloat(), state!=State::Down ? ccDarkened : ccBrightened, 4.0);
		al::DrawLine(def.rect.bottomRight().asFloat(), def.rect.topRight().asFloat(), state!=State::Down ? ccDarkened : ccBrightened, 4.0);

		DrawShadowedText(def.caption, al::White, def.rect.center(), ALLEGRO_ALIGN_CENTER);

		al::TargetBitmap.resetClippingRectangle();
	}

	bool onEvent(const al::Event& ev) override {
		bool mouseInRect = def.rect.contains(al::Vec2i{ev.mouse.x, ev.mouse.y});
		if(ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
			if(mouseInRect) {
				if(state == State::Inactive) {
					state = State::Hover;
				}
			} else {
				state = State::Inactive;
			}
		} else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			if((ev.mouse.button == al::LMB) && mouseInRect) {
				if(state == State::Hover) {
					state = State::Down;
					def.callback();
					return true;
				}
			}
		} else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
			if((ev.mouse.button == al::LMB) && mouseInRect) {
				if(state == State::Down) {
					state = State::Hover;
				}
			}
		}
		return false;
	}

	int getZOrder() override {
		return 10;
	}

private:

	[[nodiscard]] al::Color targetColor() const {
		switch (state) {
			case State::Inactive: return al::RGB(161, 196, 161);
			case State::Hover: return al::RGB(75, 200, 75);
			case State::Down: return al::RGB(65, 145, 65);
		}
		return al::Black;
	}

	State state = State::Inactive;
	al::Color currentColor = targetColor();
};

struct MessageLog: public Entity {
	std::deque<std::string> messages;
};

struct GraphGridDef {
	al::Vec2f center {};
	float zoom = 64.0;
};

struct GraphNode: public Entity {
	GraphGridDef* gridDef;
	int index = 0;
	al::Vec2f center;

	GraphNode() = default;

	void tick(double time) override {
	}

	void render() override {
	}

	bool onEvent(const al::Event &) override {
	}

	int getZOrder() override {
	}
};

struct GraphEdge: public Entity {
	GraphGridDef* gridDef;
	GraphNode* u;
	GraphNode* v;

	GraphEdge() = default;

};

struct GraphGrid: public Entity {
	using Def = GraphGridDef;
	Def def;

	GraphGrid(Def def): def(def) {}


	void tick(double time) override {
	}

	void render() override {
	}


	bool onEvent(const al::Event &) override {
		return false;
	}

	int getZOrder() override {
		return -10;
	}


	g2x::static_simple_graph get_graph() {

	}
};



int main() {

	al::Display disp(800, 600, ALLEGRO_RESIZABLE);

	al::EventLoop loop(al::DemoEventLoopConfig);
	World world;

	auto hBtn1 = world.createEntity<Button>(Button::Def{
		.rect = al::RectI::XYWH(40, 40, 200, 35),
		.caption = "This is a button",
		.callback = []() {
			//al::MessageBox("hello", "you pressed button #1");
		}
	});
	auto hBtn2 = world.createEntity<Button>(Button::Def{
		.rect = al::RectI::XYWH(40, 90, 200, 35),
		.caption = "This is another button",
		.callback = []() {
			//al::MessageBox("hello", "you pressed button #2");
		}
	});


	loop.eventDispatcher.setCatchallHandler([&](const al::Event& ev) {
		world.onEvent(ev);
		return al::EventHandled;
	});

	loop.run([&] {
		al::TargetBitmap.clearToColor(al::White);

		world.tick(loop.getLastTickTime());
		world.render();

		al::CurrentDisplay.flip();
	});
}