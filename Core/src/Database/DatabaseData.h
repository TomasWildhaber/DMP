#pragma once

namespace Core
{
	enum class DatabaseDataType
	{
		None = 0,
		Int,
		String,
		Bool,
		Enum,
		Timestamp,
	};

	struct DatabaseData
	{
		virtual inline const DatabaseDataType GetType() const { return DatabaseDataType::None; }
		virtual void* GetValue() { return nullptr; }

		virtual void Serialize(std::ostream& os) = 0;
		virtual void Deserialize(std::istream& is) = 0;
	};

	struct DatabaseInt : public DatabaseData
	{
		DatabaseInt() = default;
		DatabaseInt(int value) : Value(value) {}

		static DatabaseDataType GetStaticType() { return DatabaseDataType::Int; }
		virtual inline const DatabaseDataType GetType() const override { return GetStaticType(); }
		virtual void* GetValue() override { return &Value; }

		void Serialize(std::ostream& os) override { os.write(reinterpret_cast<const char*>(&Value), sizeof(Value)); }
		void Deserialize(std::istream& is) override { is.read(reinterpret_cast<char*>(&Value), sizeof(Value)); }

		int Value;
	};

	struct DatabaseString : public DatabaseData
	{
		DatabaseString() = default;
		DatabaseString(const char* string)
		{
			strncpy(String, string, sizeof(String) - 1);
			String[sizeof(String) - 1] = '\0';
		}

		static DatabaseDataType GetStaticType() { return DatabaseDataType::String; }
		virtual inline const DatabaseDataType GetType() const override { return GetStaticType(); }
		virtual void* GetValue() override { return &String; }

		void Serialize(std::ostream& os) override { os.write(String, sizeof(String)); }
		void Deserialize(std::istream& is) override { is.read(String, sizeof(String)); }

		char String[256];
	};

	struct DatabaseBool : public DatabaseData
	{
		DatabaseBool() = default;
		DatabaseBool(bool value) : Value(value) {}

		static DatabaseDataType GetStaticType() { return DatabaseDataType::Bool; }
		virtual inline const DatabaseDataType GetType() const override { return GetStaticType(); }
		virtual void* GetValue() override { return &Value; }

		void Serialize(std::ostream& os) override { os.write(reinterpret_cast<const char*>(&Value), sizeof(Value)); }
		void Deserialize(std::istream& is) override { is.read(reinterpret_cast<char*>(&Value), sizeof(Value)); }

		bool Value;
	};

	struct DatabaseEnum : public DatabaseData
	{
		DatabaseEnum() = default;
		DatabaseEnum(uint32_t value) : Value(value) {}

		static DatabaseDataType GetStaticType() { return DatabaseDataType::Enum; }
		virtual inline const DatabaseDataType GetType() const override { return GetStaticType(); }
		virtual void* GetValue() override { return &Value; }

		void Serialize(std::ostream& os) override { os.write(reinterpret_cast<const char*>(&Value), sizeof(Value)); }
		void Deserialize(std::istream& is) override { is.read(reinterpret_cast<char*>(&Value), sizeof(Value)); }

		uint32_t Value;
	};

	struct DatabaseTimestamp : public DatabaseData
	{
		DatabaseTimestamp() = default;

		static DatabaseDataType GetStaticType() { return DatabaseDataType::Timestamp; }
		virtual inline const DatabaseDataType GetType() const override { return GetStaticType(); }
		virtual void* GetValue() override { return nullptr; }
		// TODO: timestamp
	};
}